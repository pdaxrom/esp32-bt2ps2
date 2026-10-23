#include "esp32-ps2dev.h"
#include "ps2_timing.hpp"
#include <algorithm>
#include <climits>
#define TAG __FILE__
namespace esp32_ps2dev {
#define NOP() asm volatile("nop")
#define HIGH 1
#define LOW 0
// Timing and GPIO helpers retain the existing PS/2 waveform.

BaseType_t xTaskCreateUniversal(TaskFunction_t pxTaskCode,
                                const char *const pcName,
                                const uint32_t usStackDepth,
                                void *const pvParameters,
                                UBaseType_t uxPriority,
                                TaskHandle_t *const pxCreatedTask,
                                const BaseType_t xCoreID)
{
#ifndef CONFIG_FREERTOS_UNICORE
    if (xCoreID >= 0 && xCoreID < 2) {
        return xTaskCreatePinnedToCore(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask, xCoreID);
    } else {
#endif
        return xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask);
#ifndef CONFIG_FREERTOS_UNICORE
    }
#endif
}

extern void digitalWrite(uint8_t pin, uint8_t val)
{
    gpio_set_level((gpio_num_t)pin, val);
}

extern int digitalRead(uint8_t pin)
{
    return gpio_get_level((gpio_num_t)pin);
}
void delay(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

unsigned long IRAM_ATTR millis()
{
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
}

unsigned long IRAM_ATTR micros()
{
    return (unsigned long)(esp_timer_get_time());
}

void IRAM_ATTR delayMicroseconds(uint32_t us)
{
    const uint32_t start = micros();
    while (uint32_t(micros() - start) < us) NOP();
}

// END OF ARDUINO CORE ADAPTATION

PS2dev::PS2dev(int clk, int data)
{
    _ps2clk = clk;
    _ps2data = data;
}

void PS2dev::config(UBaseType_t task_priority, BaseType_t task_core)
{
    if (task_priority < 1) {
        task_priority = 1;
    } else if (task_priority >= configMAX_PRIORITIES) {
        task_priority = configMAX_PRIORITIES - 1;
    }
    _config_task_priority = task_priority;
    _config_task_core = task_core;
}

void PS2dev::begin(BaseType_t core = DEFAULT_TASK_CORE)
{
    gohi(_ps2clk);
    gohi(_ps2data);
    _mutex_state = xSemaphoreCreateRecursiveMutex();
    _mutex_bus = xSemaphoreCreateMutex();
    _queue_packet = xQueueCreate(PACKET_QUEUE_LENGTH, sizeof(PS2Packet));
    ESP_ERROR_CHECK(_mutex_state && _mutex_bus && _queue_packet ? ESP_OK : ESP_ERR_NO_MEM);
    _config_task_core = core;
}
void PS2dev::start_bus_task()
{
    ESP_ERROR_CHECK(xTaskCreateUniversal(_taskfn_process_host_request, "process_host_request", 4096, this, _config_task_priority,
                         &_task_process_host_request, _config_task_core) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}

void PS2dev::gohi(int pin)
{
    digitalWrite(pin, HIGH);
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
}
void PS2dev::golo(int pin)
{
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT_OD);
    digitalWrite(pin, LOW);
}
void PS2dev::ack()
{
    delayMicroseconds(BYTE_INTERVAL_MICROS);
    send_byte(0xFA);
    delayMicroseconds(BYTE_INTERVAL_MICROS);
}
int PS2dev::write(unsigned char data)
{
    if (get_bus_state() != BusState::IDLE) return -1;
    const uint8_t sent = data;
    uint8_t parity = 1;
    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    taskENTER_CRITICAL(&mux);
    const auto clock = [&] {
        golo(_ps2clk);
        delayMicroseconds(CLK_HALF_PERIOD_MICROS);
        gohi(_ps2clk);
        delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
        return digitalRead(_ps2clk) == HIGH;
    };
    const auto abort = [&] {
        // The host owns CLOCK. Release the bus and retry this byte later.
        gohi(_ps2data); gohi(_ps2clk);
        taskEXIT_CRITICAL(&mux);
        return -1;
    };
    golo(_ps2data);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    if (!clock()) return abort();
    for (unsigned bit = 0; bit < 8; ++bit) {
        if (data & 1) gohi(_ps2data); else golo(_ps2data);
        delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
        if (!clock()) return abort();
        parity ^= data & 1;
        data >>= 1;
    }
    if (parity) gohi(_ps2data); else golo(_ps2data);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    if (!clock()) return abort();
    gohi(_ps2data); // Stop bit.
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    // The host has received the byte at the stop falling edge. Its
    // inhibit after that edge is backpressure for the next byte.
    // Retrying here duplicates an already delivered byte.
    clock();
    taskEXIT_CRITICAL(&mux);
    if (sent != 0xFE) { _last_byte = sent; _has_last_byte = true; }
    return 0;
}
int PS2dev::write_wait_idle(uint8_t data, uint64_t timeout_micros)
{
    const int64_t start_time = esp_timer_get_time();
    while (get_bus_state() != BusState::IDLE) {
        if (uint64_t(esp_timer_get_time() - start_time) >= timeout_micros) {
            return -1;
        }
    }
    return write(data);
}
int PS2dev::read(unsigned char *value, uint64_t timeout_ms)
{
    unsigned int data = 0x00;
    unsigned int bit = 0x01;

    unsigned char calculated_parity = 1;
    unsigned char received_parity = 0;

    // wait for data line to go low and clock line to go high (or timeout)
    const int64_t waiting_since = esp_timer_get_time();
    while (get_bus_state() != BusState::HOST_REQUEST_TO_SEND) {
        if (uint64_t((esp_timer_get_time() - waiting_since) / 1000) >= timeout_ms) {
            return -1;
        }
        delay(1);
    }

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    taskENTER_CRITICAL(&mux);

    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    golo(_ps2clk);
    delayMicroseconds(CLK_HALF_PERIOD_MICROS);
    gohi(_ps2clk);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

    while (bit < 0x0100) {
        if (digitalRead(_ps2data) == HIGH) {
            data = data | bit;
            calculated_parity = calculated_parity ^ 1;
        } else {
            calculated_parity = calculated_parity ^ 0;
        }

        bit = bit << 1;

        delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
        golo(_ps2clk);
        delayMicroseconds(CLK_HALF_PERIOD_MICROS);
        gohi(_ps2clk);
        delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    }
    // we do the delay at the end of the loop, so at this point we have
    // already done the delay for the parity bit

    // parity bit
    if (digitalRead(_ps2data) == HIGH) {
        received_parity = 1;
    }

    // stop bit
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    golo(_ps2clk);
    delayMicroseconds(CLK_HALF_PERIOD_MICROS);
    gohi(_ps2clk);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

    const bool valid_stop = digitalRead(_ps2data) == HIGH;

    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    golo(_ps2data);
    golo(_ps2clk);
    delayMicroseconds(CLK_HALF_PERIOD_MICROS);
    gohi(_ps2clk);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    gohi(_ps2data);

    taskEXIT_CRITICAL(&mux);

    *value = data & 0x00FF;

    if (valid_stop && received_parity == calculated_parity) {
        return 0;
    } else {
        return -2;
    }
}
PS2dev::BusState PS2dev::get_bus_state()
{
    if (digitalRead(_ps2clk) == LOW) {
        return BusState::COMMUNICATION_INHIBITED;
    } else if (digitalRead(_ps2data) == LOW) {
        return BusState::HOST_REQUEST_TO_SEND;
    } else {
        return BusState::IDLE;
    }
}
SemaphoreHandle_t PS2dev::get_bus_mutex_handle()
{
    return _mutex_bus;
}
QueueHandle_t PS2dev::get_packet_queue_handle()
{
    return _queue_packet;
}
int PS2dev::send_packet(const PS2Packet *packet)
{
    if (!packet || packet->len > sizeof(packet->data)) return -1;
    if (!packet->len) return 0;
    auto queued = *packet;
    queued.generation = _packet_generation.load();
    return (xQueueSend(_queue_packet, &queued, 0) == pdTRUE) ? 0 : -1;
}

void PS2dev::discard_queued_packets()
{
    ++_packet_generation;
    xQueueReset(_queue_packet);
}
void PS2dev::remember_packet(const PS2Packet &packet)
{
    if (!(packet.len == 1 && packet.data[0] == 0xFE)) _last_packet = packet;
}
bool PS2dev::send_response(const PS2Packet &packet)
{
    if (!packet.len || packet.len > sizeof(packet.data) || _response_count == _responses.size()) return false;
    _responses[_response_count++] = packet;
    flush_responses();
    return true;
}
void PS2dev::send_byte(uint8_t byte)
{
    PS2Packet packet{}; packet.len = 1; packet.data[0] = byte;
    const bool queued = send_response(packet);
    configASSERT(queued);
    if (!queued) ESP_LOGE(TAG, "PS/2 response buffer full");
}
void PS2dev::flush_responses()
{
    while (_response_count) {
        const auto &packet = _responses[0];
        while (_response_next < packet.len) {
            if (write(packet.data[_response_next]) != 0) return;
            ++_response_next;
            delayMicroseconds(BYTE_INTERVAL_MICROS);
        }
        remember_packet(packet);
        --_response_count;
        for (size_t i = 0; i < _response_count; ++i) _responses[i] = _responses[i + 1];
        _response_next = 0;
    }
}
void PS2dev::resend_byte()
{
    if (_has_last_byte) send_byte(_last_byte);
}
void PS2dev::resend_packet()
{
    const auto packet = _last_packet;
    if (packet.len) send_response(packet);
}

void PS2dev::service_bus()
{
    xSemaphoreTake(_mutex_bus, portMAX_DELAY);
    if (get_bus_state() == BusState::HOST_REQUEST_TO_SEND) {
        uint8_t command;
        const int result = read(&command);
        // A new host request supersedes any interrupted response.
        _response_count = _response_next = 0;
        if (result == 0) {
            reply_to_host(command);
        } else if (result == -2) send_byte(0xFE);
    }
    flush_responses();
    if (!_response_count && !_pending_command) {
        if (!_has_stream_packet)
            _has_stream_packet = xQueueReceive(_queue_packet, &_stream_packet, 0) == pdTRUE;
        if (_has_stream_packet && _stream_packet.generation != packet_generation()) {
            _has_stream_packet = false;
            _stream_next = 0;
        }
        if (_has_stream_packet) {
            delayMicroseconds(BYTE_INTERVAL_MICROS);
            while (_stream_next < _stream_packet.len && write(_stream_packet.data[_stream_next]) == 0) {
                ++_stream_next;
                delayMicroseconds(BYTE_INTERVAL_MICROS);
            }
            if (_stream_next == _stream_packet.len) {
                remember_packet(_stream_packet);
                _has_stream_packet = false;
                _stream_next = 0;
            }
        }
    }
    xSemaphoreGive(_mutex_bus);
}
void _taskfn_process_host_request(void *arg)
{
    auto *device = static_cast<PS2dev *>(arg);
    for (;;) {
        device->service_bus();
        // Bounded polling keeps commands responsive even while transmission is inhibited.
        delay(1);
    }
}

} // namespace esp32_ps2dev
