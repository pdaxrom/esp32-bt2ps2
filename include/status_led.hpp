#pragma once
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include <atomic>
#include <cstdint>

// One task owns the status GPIO, including the complete six-digit Passkey display.
class StatusLed {
public:
    bool begin();
    void show_passkey(uint32_t code);
    void pairing(bool active) { pairing_.store(active); }
    void activity();
private:
    static void task(void *arg);
    void run();
    QueueHandle_t codes_ = nullptr;
    std::atomic<bool> pairing_{false};
    std::atomic<int64_t> activity_until_{0};
};
