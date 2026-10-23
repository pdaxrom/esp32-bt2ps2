#ifndef __ESP32_PS2DEV_H__
#define __ESP32_PS2DEV_H__

#include <initializer_list>
#include <stack>
#include "freertos_lock.hpp"
#include "input_output.hpp"
#include "ps2_packet.hpp"
#include <array>
#include <atomic>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include <stdint.h>
#include "scan_codes.h"

namespace esp32_ps2dev
{
// Time per clock should be 60 to 100 microseconds according to PS/2 specifications.
// Thus, half period should be 30 to 50 microseconds.
const uint32_t CLK_HALF_PERIOD_MICROS = 40;
const uint32_t CLK_QUATER_PERIOD_MICROS = CLK_HALF_PERIOD_MICROS / 2;
// I could not find any specification of time between bytes from the PS/2 specification.
// Based on observation of the mouse signal waveform using an oscilloscope, there appears to be an interval of 1 to 2 clock cycles.
// ref. https://youtu.be/UqRDLWGLCEk
const uint32_t BYTE_INTERVAL_MICROS = 500; // in v0.4 was OK: 500, change if not working and you know what you're doing.
const int PACKET_QUEUE_LENGTH = 20;
const UBaseType_t DEFAULT_TASK_PRIORITY = 10;
//const BaseType_t DEFAULT_TASK_CORE = APP_CPU_NUM;
const BaseType_t DEFAULT_TASK_CORE = 0;
const BaseType_t DEFAULT_TASK_CORE_MOUSE = 1;
// The device should check for "HOST_REQUEST_TO_SEND" at a interval not exceeding 10 milliseconds.
const uint32_t INTERVAL_CHECKING_HOST_SEND_REQUEST_MILLIS = 9;
const uint32_t MOUSE_CLICK_PRESSING_DURATION_MILLIS = 100;

class PS2dev
{
public:
    PS2dev(int clk, int data);

    enum class BusState {
        IDLE,
        COMMUNICATION_INHIBITED,
        HOST_REQUEST_TO_SEND,
    };

    void config(UBaseType_t task_priority, BaseType_t task_core);
    void begin(BaseType_t core);
    int write(unsigned char data);
    int write_wait_idle(uint8_t data, uint64_t timeout_micros = 1500);
    int read(unsigned char *data, uint64_t timeout_ms = 0);
    virtual int reply_to_host(uint8_t host_cmd) = 0;
    BusState get_bus_state();
    SemaphoreHandle_t get_bus_mutex_handle();
    QueueHandle_t get_packet_queue_handle();
    int send_packet(const PS2Packet *packet);
    void remember_packet(const PS2Packet &packet);
    void service_bus();
    uint32_t packet_generation() const { return _packet_generation.load(); }


protected:
    int _ps2clk;
    int _ps2data;
    UBaseType_t _config_task_priority = DEFAULT_TASK_PRIORITY;
    BaseType_t _config_task_core = DEFAULT_TASK_CORE;
    TaskHandle_t _task_process_host_request = nullptr;
    QueueHandle_t _queue_packet = nullptr;
    SemaphoreHandle_t _mutex_bus = nullptr;
    void golo(int pin);
    void gohi(int pin);
    void ack();
    void start_bus_task();
    bool send_response(const PS2Packet &packet);
    void send_byte(uint8_t byte);
    void flush_responses();
    void resend_byte();
    void resend_packet();
    void discard_queued_packets();
    std::atomic<uint32_t> _packet_generation{0};
    SemaphoreHandle_t _mutex_state = nullptr;
    PS2Packet _last_packet{};
    uint8_t _last_byte = 0;
    bool _has_last_byte = false;
    // Bus-owner state: command replies always precede ordinary input packets.
    std::array<PS2Packet, 4> _responses{};
    uint8_t _response_count = 0, _response_next = 0;
    PS2Packet _stream_packet{};
    uint8_t _stream_next = 0;
    bool _has_stream_packet = false;
    uint8_t _pending_command = 0;

};

class PS2Mouse : public PS2dev
{
public:
    PS2Mouse(int clk, int data);
    enum class ResolutionCode : uint8_t {
        RES_1 = 0x00,
        RES_2 = 0x01,
        RES_4 = 0x02,
        RES_8 = 0x03
    };
    enum class Scale : uint8_t {
        ONE_ONE = 0,
        TWO_ONE = 1
    };
    enum class Mode : uint8_t {
        REMOTE_MODE = 0,
        STREAM_MODE = 1,
        WRAP_MODE = 2
    };
    enum class Command : uint8_t {
        RESET = 0xFF,
        RESEND = 0xFE,
        ERROR = 0xFC,
        ACK = 0xFA,
        SET_DEFAULTS = 0xF6,
        DISABLE_DATA_REPORTING = 0xF5,
        ENABLE_DATA_REPORTING = 0xF4,
        SET_SAMPLE_RATE = 0xF3,
        GET_DEVICE_ID = 0xF2,
        SET_REMOTE_MODE = 0xF0,
        SET_WRAP_MODE = 0xEE,
        RESET_WRAP_MODE = 0xEC,
        READ_DATA = 0xEB,
        SET_STREAM_MODE = 0xEA,
        STATUS_REQUEST = 0xE9,
        SET_RESOLUTION = 0xE8,
        SET_SCALING_2_1 = 0xE7,
        SET_SCALING_1_1 = 0xE6,
        SELF_TEST_PASSED = 0xAA,
    };
    enum class Button : uint8_t {
        LEFT,
        RIGHT,
        MIDDLE,
        BUTTON_4,
        BUTTON_5,
    };

    void begin();
    int reply_to_host(uint8_t host_cmd);
    bool has_wheel();
    bool has_4th_and_5th_buttons();
    bool data_reporting_enabled();
    void reset_counter();
    uint8_t get_sample_rate();
    void move(int64_t x, int64_t y, int64_t wheel);
    void apply_input(int64_t x, int64_t y, int64_t wheel, uint8_t buttons);
    void press(Button button);
    void release(Button button);
    void click(Button button);
    void _report(bool response = false);
    bool stream_reporting_enabled();

protected:
    static constexpr char const *TAG = "PS2Mouse";
    void _send_status();
    TaskHandle_t _task_poll_mouse_count = nullptr;
    bool _has_wheel = false;
    bool _has_4th_and_5th_buttons = false;
    bool _data_reporting_enabled = false;
    ResolutionCode _resolution = ResolutionCode::RES_4;
    Scale _scale = Scale::ONE_ONE;
    Mode _mode = Mode::STREAM_MODE;
    Mode _last_mode = Mode::STREAM_MODE;
    uint8_t _last_sample_rate[3] = {0, 0, 0};
    uint8_t _sample_rate = 100;
    int32_t _count_x = 0;
    int32_t _count_y = 0;
    int32_t _count_z = 0;
    bool _buttons_dirty = true;
    std::array<uint8_t, PACKET_QUEUE_LENGTH> _button_events{};
    uint8_t _button_event_count = 0;
    uint8_t _button_left = 0;
    uint8_t _button_right = 0;
    uint8_t _button_middle = 0;
    uint8_t _button_4th = 0;
    uint8_t _button_5th = 0;
};

class PS2Keyboard : public PS2dev
{
public:
    PS2Keyboard(int clk, int data);
    int reply_to_host(uint8_t host_cmd);
    enum class Command {
        RESET = 0xFF,
        RESEND = 0xFE,
        ACK = 0xFA,
        SET_SPECIFIC_KEY_TO_MAKE_ONLY = 0xFD,
        SET_SPECIFIC_KEY_TO_MAKE_RELEASE = 0xFC,
        SET_SPECIFIC_KEY_TO_TYPEMATIC_AUTOREPEAT_ONLY=0xFB,
        SET_ALL_KEYS_TO_TYPEMATIC_AUTOREPEAT_MAKE_RELEASE = 0xFA,
        SET_ALL_KEYS_TO_MAKE_ONLY = 0xF9,
        SET_ALL_KEYS_TO_MAKE_RELEASE = 0xF8,
        SET_ALL_KEYS_TO_TYPEMATIC_AUTOREPEAT_ONLY = 0xF7,
        SET_DEFAULTS = 0xF6,
        DISABLE_DATA_REPORTING = 0xF5,
        ENABLE_DATA_REPORTING = 0xF4,
        SET_TYPEMATIC_RATE = 0xF3,
        GET_DEVICE_ID = 0xF2,
        SET_SCAN_CODE_SET = 0xF0,
        ECHO = 0xEE,
        SET_RESET_LEDS = 0xED,
        BAT_SUCCESS = 0xAA,
    };

    enum KeyLed: uint8_t {
        KEYBOARD_LED_SCROLLLOCK = 1 << 0,
        KEYBOARD_LED_NUMLOCK = 1 << 1,
        KEYBOARD_LED_CAPSLOCK = 1 << 2,
    };

    struct KeyBehavior {
        bool make = true;
        bool break_code = true;
        bool typematic = true;
    };

public:
    typedef void (*_leds_callback)(uint8_t);

    void begin();
    bool data_reporting_enabled();
    bool is_scroll_lock_led_on();
    bool is_num_lock_led_on();
    bool is_caps_lock_led_on();
    bool keydown(scancodes::Key key);
    bool keyup(scancodes::Key key);
    void type(scancodes::Key key);
    void type(std::initializer_list<scancodes::Key> keys);
    void type(const char *str);
    bool keyHid_send(uint8_t btkey, bool keyDown);
    bool keyHid_send_CCONTROL(uint16_t btkey, bool keyDown);
    static uint8_t canonical_hid_key(uint8_t code);
    static uint8_t consumer_hid_key(uint16_t code);
    bool allows_typematic(uint8_t hid_code) const;
    void begin_input_update() { xSemaphoreTakeRecursive(_mutex_state, portMAX_DELAY); }
    void end_input_update() { xSemaphoreGiveRecursive(_mutex_state); }
    uint16_t get_typematic_delay_ms() const
    {
        RecursiveLock lock(_mutex_state);
        return _typematic_delay_ms;
    }
    uint16_t get_typematic_cycle_ms() const
    {
        RecursiveLock lock(_mutex_state);
        return _typematic_cycle_ms;
    }
    Typematic typematic() const {
        RecursiveLock lock(_mutex_state);
        return {_typematic_config, _typematic_delay_ms, _typematic_cycle_ms};
    }
    uint8_t get_typematic_config() const
    {
        RecursiveLock lock(_mutex_state);
        return _typematic_config;
    }

    void set_leds_callback(_leds_callback cb)
    {
        leds_callback = cb;
    }
    void trigger_leds_callback(uint8_t leds)
    {
        if (leds_callback != nullptr) {
            leds_callback(leds);
        }
    }

protected:
    bool _data_reporting_enabled = true;
    bool _led_scroll_lock = false;
    bool _led_num_lock = false;
    bool _led_caps_lock = false;
    int _scan_code_set = 2;
    std::array<KeyBehavior, scancodes::KEY_COUNT> _key_behaviors{};
    void apply_behavior_to_all(const KeyBehavior &behavior);
    void apply_default_key_behaviors();
    void set_key_behavior(scancodes::Key key, const KeyBehavior &behavior);
    void configure_specific_key(uint8_t scan_code, const KeyBehavior &behavior);
    const KeyBehavior &behavior_for_key(scancodes::Key key) const;
    bool allows_typematic_for_key(scancodes::Key key) const;
    void update_typematic(uint8_t config);
    uint8_t _typematic_config = 0x2B; // default 500ms delay, ~10.9cps
    uint16_t _typematic_delay_ms = 500;
    uint16_t _typematic_cycle_ms = 92;
    _leds_callback leds_callback = nullptr;
};

void _taskfn_process_host_request(void *arg);
void _taskfn_poll_mouse_count(void *arg);

} // namespace esp32_ps2dev

#endif // __ESP32_PS2DEV_H__
