#include "connection_manager.hpp"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"

bool ConnectionManager::begin(bool pair_on_start)
{
    commands_ = xQueueCreate(8, sizeof(Command));
    if (!commands_) return false;
    if (pair_on_start) { Command command = Command::Pair; xQueueSend(commands_, &command, 0); }
    return xTaskCreate(task, "connections", 4096, this, 1, nullptr) == pdPASS;
}
void ConnectionManager::poll_button(int64_t now_ms)
{
    const bool down = !gpio_get_level(GPIO_NUM_0);
    if (down && !button_down_) {
        pressed_at_ = now_ms;
        cancel_sent_ = false;
        if (mode_.load() == Mode::KnownDevices) {
            Command command = Command::Pair;
            if (xQueueSend(commands_, &command, 0) != pdTRUE) ESP_LOGW("Connections", "Pairing request queue full");
        }
    }
    if (down && !cancel_sent_ && now_ms - pressed_at_ >= 2000) {
        Command command = Command::Cancel;
        if (xQueueSend(commands_, &command, 0) == pdTRUE) cancel_sent_ = true;
    }
    button_down_ = down;
}
void ConnectionManager::task(void *arg) { static_cast<ConnectionManager *>(arg)->run(); }
void ConnectionManager::run()
{
    int64_t next_scan = 0;
    uint32_t pairing_generation = 0;
    for (;;) {
        Command command;
        if (xQueueReceive(commands_, &command, pdMS_TO_TICKS(100)) == pdTRUE) {
            mode_ = command == Command::Pair ? Mode::Pairing : Mode::KnownDevices;
            pairing_generation = BluetoothHost::connection_generation.load();
            next_scan = 0;
            led_.pairing(mode_ == Mode::Pairing);
        }
        if (mode_ == Mode::Pairing && BluetoothHost::connection_generation.load() != pairing_generation) {
            mode_ = Mode::KnownDevices;
            led_.pairing(false);
            next_scan = 0;
        }
        if (esp_timer_get_time() < next_scan) continue;
        if (mode_ == Mode::Pairing) {
            host_.scan_pairing_devices();
            const int64_t pause = BluetoothHost::pairing_pending.exchange(false) ? 60000000 : 1000000;
            next_scan = esp_timer_get_time() + pause;
        } else {
            host_.scan_known_devices();
            next_scan = esp_timer_get_time() + 1000000;
        }
    }
}
