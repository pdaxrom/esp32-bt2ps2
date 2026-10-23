#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "ps2_test_bus.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

struct FakeSemaphore { std::recursive_mutex mutex; };
struct FakeQueue {
    unsigned capacity, size;
    std::deque<std::vector<uint8_t>> data;
    std::mutex mutex;
};
namespace {
std::vector<std::unique_ptr<FakeSemaphore>> semaphores;
std::vector<std::unique_ptr<FakeQueue>> queues;
std::atomic<int64_t> time_us{0};
std::array<bool, 40> released{};
std::array<unsigned, 40> levels{};
int clock_pin = 0, data_pin = 1;
bool host_clock = false, host_data = false, receive = false;
bool inhibit_pending = false, request_pending = false, request_armed = false;
unsigned stop_after = 0, stop_bit = 0, inhibit_stop_count = 0;
std::vector<bool> falling_bits;
std::vector<uint8_t> stop_bytes;
uint8_t request_byte = 0;
std::vector<bool> tx_bits, rx_bits;
size_t rx_index = 0;
std::vector<uint8_t> sent;
bool device_high(int pin) { return released[pin] || levels[pin]; }
}
SemaphoreHandle_t xSemaphoreCreateMutex()
{
    semaphores.push_back(std::make_unique<FakeSemaphore>()); return semaphores.back().get();
}
SemaphoreHandle_t xSemaphoreCreateRecursiveMutex() { return xSemaphoreCreateMutex(); }
BaseType_t xSemaphoreTake(SemaphoreHandle_t mutex, TickType_t) { mutex->mutex.lock(); return pdTRUE; }
BaseType_t xSemaphoreGive(SemaphoreHandle_t mutex) { mutex->mutex.unlock(); return pdTRUE; }
BaseType_t xSemaphoreTakeRecursive(SemaphoreHandle_t mutex, TickType_t timeout) { return xSemaphoreTake(mutex, timeout); }
BaseType_t xSemaphoreGiveRecursive(SemaphoreHandle_t mutex) { return xSemaphoreGive(mutex); }
QueueHandle_t xQueueCreate(unsigned capacity, unsigned size)
{
    auto queue = std::make_unique<FakeQueue>(); queue->capacity = capacity; queue->size = size;
    queues.push_back(std::move(queue)); return queues.back().get();
}
BaseType_t xQueueSend(QueueHandle_t queue, const void *data, TickType_t)
{
    std::lock_guard<std::mutex> lock(queue->mutex);
    if (queue->data.size() == queue->capacity) return pdFALSE;
    const auto *bytes = static_cast<const uint8_t *>(data);
    queue->data.emplace_back(bytes, bytes + queue->size); return pdTRUE;
}
BaseType_t xQueueReceive(QueueHandle_t queue, void *data, TickType_t)
{
    std::lock_guard<std::mutex> lock(queue->mutex);
    if (queue->data.empty()) return pdFALSE;
    std::memcpy(data, queue->data.front().data(), queue->size); queue->data.pop_front(); return pdTRUE;
}
BaseType_t xQueueReset(QueueHandle_t queue)
{
    std::lock_guard<std::mutex> lock(queue->mutex); queue->data.clear(); return pdTRUE;
}
BaseType_t xTaskCreate(TaskFunction_t, const char *, uint32_t, void *, UBaseType_t, TaskHandle_t *) { return pdPASS; }
BaseType_t xTaskCreatePinnedToCore(TaskFunction_t, const char *, uint32_t, void *, UBaseType_t, TaskHandle_t *, BaseType_t) { return pdPASS; }
void vTaskDelay(TickType_t ticks) { time_us += ticks * 1000; }
void vTaskDelete(TaskHandle_t) {}
int64_t esp_timer_get_time() { return time_us.fetch_add(1); }
void fake_enter_critical() {}
void fake_exit_critical()
{
    if (inhibit_pending) { host_clock = true; inhibit_pending = false; }
    if (request_pending) { request_pending = false; test_bus::request(request_byte); }
}
int gpio_get_level(gpio_num_t pin)
{
    return device_high(pin) && !(pin == clock_pin && host_clock) && !(pin == data_pin && host_data);
}
namespace {
void observe_edge(gpio_num_t pin, bool was_high)
{
    // A real host samples at falling edges and may inhibit immediately
    // after accepting the stop bit, before the device's last CLOCK read.
    if (pin == clock_pin && was_high && !device_high(pin) &&
        !host_clock && !receive && inhibit_stop_count) {
        falling_bits.push_back(gpio_get_level(data_pin));
        if (falling_bits.size() == 11) {
            assert(!falling_bits[0] && falling_bits[10]);
            uint8_t byte = 0; unsigned parity = falling_bits[9];
            for (unsigned bit = 0; bit < 8; ++bit) {
                byte |= falling_bits[bit + 1] << bit;
                parity += falling_bits[bit + 1];
            }
            assert(parity % 2 == 1);
            stop_bytes.push_back(byte); falling_bits.clear();
            if (--inhibit_stop_count == 0) host_clock = true;
        }
    }
    if (pin != clock_pin || was_high || !device_high(pin) || host_clock) return;
    if (receive) {
        if (rx_index < rx_bits.size()) host_data = !rx_bits[rx_index++];
        else { receive = false; host_data = false; }
        if (rx_index == rx_bits.size()) { receive = false; host_data = false; }
        return;
    }
    tx_bits.push_back(gpio_get_level(data_pin));
    if (stop_bit && tx_bits.size() == stop_bit) { host_clock = true; stop_bit = 0; return; }
    if (tx_bits.size() == 11) {
        assert(!tx_bits[0] && tx_bits[10]);
        uint8_t byte = 0; unsigned parity = tx_bits[9];
        for (unsigned bit = 0; bit < 8; ++bit) { byte |= tx_bits[bit + 1] << bit; parity += tx_bits[bit + 1]; }
        assert(parity % 2 == 1); sent.push_back(byte); tx_bits.clear();
        if (stop_after && sent.size() == stop_after) {
            if (request_armed) { request_pending = true; request_armed = false; }
            else inhibit_pending = true;
            stop_after = 0;
        }
    }
}
}
int gpio_set_level(gpio_num_t pin, unsigned level)
{
    const bool was_high = device_high(pin); levels.at(pin) = level;
    observe_edge(pin, was_high); return 0;
}
int gpio_set_direction(gpio_num_t pin, int mode)
{
    const bool was_high = device_high(pin); released.at(pin) = mode == GPIO_MODE_INPUT;
    if (pin == data_pin && released[pin] && host_clock) tx_bits.clear();
    observe_edge(pin, was_high); return 0;
}
namespace test_bus {
void select(int clock, int data)
{
    clock_pin = clock; data_pin = data;
    released.fill(true); levels.fill(0); clear();
}
void inhibit(bool active) { host_clock = active; if (!active) tx_bits.clear(); }
void inhibit_after(unsigned complete_bytes) { stop_after = sent.size() + complete_bytes; }
void inhibit_at_bit(unsigned bit) { stop_bit = bit; }
void inhibit_stop_after(unsigned bytes) {
    inhibit_stop_count = bytes; falling_bits.clear(); stop_bytes.clear();
}
const std::vector<uint8_t> &stop_received() { return stop_bytes; }
void request(uint8_t byte, bool bad_parity, bool bad_stop)
{
    host_clock = false; host_data = true; receive = true; rx_index = 0; rx_bits.clear();
    bool parity = true;
    for (unsigned bit = 0; bit < 8; ++bit) { const bool value = (byte >> bit) & 1; rx_bits.push_back(value); parity ^= value; }
    rx_bits.push_back(parity ^ bad_parity); rx_bits.push_back(!bad_stop); rx_bits.push_back(true);
}
void request_after(unsigned complete_bytes, uint8_t byte)
{
    request_byte = byte;
    // The completed frame triggers the request in fake_exit_critical.
    stop_after = sent.size() + complete_bytes;
    request_armed = true;
}
void clear()
{
    sent.clear(); tx_bits.clear(); rx_bits.clear(); rx_index = 0;
    host_clock = host_data = receive = inhibit_pending = request_pending = request_armed = false;
    stop_after = stop_bit = inhibit_stop_count = 0;
    falling_bits.clear(); stop_bytes.clear();
}
const std::vector<uint8_t> &bytes() { return sent; }
void time(int64_t microseconds) { time_us = microseconds; }
}
