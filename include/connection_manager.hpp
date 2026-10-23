#pragma once
#include "bluetooth_host.hpp"
#include "status_led.hpp"

// The connection task alone scans; pairing requests and cancellation are messages.
class ConnectionManager {
public:
    ConnectionManager(BluetoothHost &host, StatusLed &led) : host_(host), led_(led) {}
    bool begin(bool pair_on_start);
    void poll_button(int64_t now_ms);
private:
    enum class Command : uint8_t { Pair, Cancel };
    enum class Mode : uint8_t { KnownDevices, Pairing };
    static void task(void *arg);
    void run();
    BluetoothHost &host_;
    StatusLed &led_;
    QueueHandle_t commands_ = nullptr;
    std::atomic<Mode> mode_{Mode::KnownDevices};
    bool button_down_ = false, cancel_sent_ = false;
    int64_t pressed_at_ = 0;
};
