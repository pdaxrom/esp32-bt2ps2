#include "esp32-ps2dev.h"
#include "ps2_timing.hpp"
#include "ps2_mouse_report.hpp"
#include <algorithm>
#include <climits>
#define TAG __FILE__
namespace esp32_ps2dev {
PS2Mouse::PS2Mouse(int clk, int data) : PS2dev(clk, data) { _config_task_core = DEFAULT_TASK_CORE_MOUSE; }
void PS2Mouse::begin()
{
    PS2dev::begin(_config_task_core);
    delay(200);
    PS2Packet bat{}; bat.len = 2; bat.data[0] = 0xAA; bat.data[1] = 0;
    send_response(bat);
    start_bus_task();
    ESP_ERROR_CHECK(xTaskCreatePinnedToCore(_taskfn_poll_mouse_count, "PS2Mouse", 4096, this, _config_task_priority - 1,
                            &_task_poll_mouse_count, _config_task_core) == pdPASS ? ESP_OK : ESP_ERR_NO_MEM);
}
int PS2Mouse::reply_to_host(uint8_t value)
{
    RecursiveLock lock(_mutex_state);
    if (_mode == Mode::WRAP_MODE && value != static_cast<uint8_t>(Command::RESET)) {
        if (value == static_cast<uint8_t>(Command::RESET_WRAP_MODE)) {
            discard_queued_packets(); reset_counter(); _mode = _last_mode; ack();
        } else send_byte(value);
        return 0;
    }
    if (_pending_command && value < static_cast<uint8_t>(Command::SET_SCALING_1_1)) {
        if (_pending_command == static_cast<uint8_t>(Command::SET_SAMPLE_RATE)) {
            if (value != 10 && value != 20 && value != 40 && value != 60 && value != 80 && value != 100 && value != 200) {
                send_byte(0xFE); return 0;
            }
            _sample_rate = value;
            _last_sample_rate[0] = _last_sample_rate[1];
            _last_sample_rate[1] = _last_sample_rate[2];
            _last_sample_rate[2] = value;
            if (_last_sample_rate[0] == 200 && _last_sample_rate[1] == 100 && value == 80) _has_wheel = true;
            else if (_has_wheel && _last_sample_rate[0] == 200 && _last_sample_rate[1] == 200 && value == 80)
                _has_4th_and_5th_buttons = true;
        } else {
            if (value > 3) { send_byte(0xFE); return 0; }
            _resolution = static_cast<ResolutionCode>(value);
        }
        _pending_command = 0; reset_counter(); ack();
        return 0;
    }
    _pending_command = 0;
    switch (static_cast<Command>(value)) {
    case Command::RESET: {
        discard_queued_packets();
        _has_wheel = _has_4th_and_5th_buttons = false;
        std::fill(std::begin(_last_sample_rate), std::end(_last_sample_rate), 0);
        _sample_rate = 100; _resolution = ResolutionCode::RES_4; _scale = Scale::ONE_ONE;
        _data_reporting_enabled = false; _mode = _last_mode = Mode::STREAM_MODE;
        reset_counter(); _buttons_dirty = true;
        ack();
        PS2Packet bat{}; bat.len = 2; bat.data[0] = 0xAA; bat.data[1] = 0;
        send_response(bat); break;
    }
    case Command::RESEND: resend_packet(); break;
    case Command::SET_DEFAULTS:
        discard_queued_packets();
        _sample_rate = 100; _resolution = ResolutionCode::RES_4; _scale = Scale::ONE_ONE;
        std::fill(std::begin(_last_sample_rate), std::end(_last_sample_rate), 0);
        _data_reporting_enabled = false; _mode = Mode::STREAM_MODE;
        reset_counter(); _buttons_dirty = true; ack(); break;
    case Command::DISABLE_DATA_REPORTING:
        discard_queued_packets(); _data_reporting_enabled = false;
        reset_counter(); _buttons_dirty = true; ack(); break;
    case Command::ENABLE_DATA_REPORTING:
        _data_reporting_enabled = true; reset_counter(); _buttons_dirty = true; ack(); break;
    case Command::SET_SAMPLE_RATE:
    case Command::SET_RESOLUTION:
        _pending_command = value; ack(); break;
    case Command::GET_DEVICE_ID:
        ack(); send_byte(_has_4th_and_5th_buttons ? 4 : _has_wheel ? 3 : 0); reset_counter(); break;
    case Command::SET_REMOTE_MODE:
        discard_queued_packets(); reset_counter(); _mode = Mode::REMOTE_MODE; ack(); break;
    case Command::SET_WRAP_MODE:
        discard_queued_packets(); reset_counter(); _last_mode = _mode; _mode = Mode::WRAP_MODE; ack(); break;
    case Command::RESET_WRAP_MODE: ack(); break;
    case Command::READ_DATA: ack(); _report(true); break;
    case Command::SET_STREAM_MODE:
        discard_queued_packets(); reset_counter(); _mode = Mode::STREAM_MODE; _buttons_dirty = true; ack(); break;
    case Command::STATUS_REQUEST: ack(); _send_status(); break;
    case Command::SET_SCALING_2_1: reset_counter(); _scale = Scale::TWO_ONE; ack(); break;
    case Command::SET_SCALING_1_1: reset_counter(); _scale = Scale::ONE_ONE; ack(); break;
    default: send_byte(0xFE); break;
    }
    return 0;
}
bool PS2Mouse::has_wheel()
{
    RecursiveLock lock(_mutex_state);
    return _has_wheel;
}
bool PS2Mouse::has_4th_and_5th_buttons()
{
    RecursiveLock lock(_mutex_state);
    return _has_4th_and_5th_buttons;
}
bool PS2Mouse::data_reporting_enabled()
{
    RecursiveLock lock(_mutex_state);
    return _data_reporting_enabled;
}
void PS2Mouse::reset_counter()
{
    RecursiveLock lock(_mutex_state);
    _count_x = 0;
    _count_y = 0;
    _count_z = 0;
    _button_event_count = 0;
    _buttons_dirty = true;
}
uint8_t PS2Mouse::get_sample_rate()
{
    RecursiveLock lock(_mutex_state);
    return _sample_rate;
}
void PS2Mouse::move(int64_t x, int64_t y, int64_t wheel)
{
    RecursiveLock lock(_mutex_state);
    // Saturate before conversion, so even a long inhibited interval can't overflow.
    const auto accumulate = [](int32_t current, int64_t delta) {
        delta = std::clamp<int64_t>(delta, -INT32_MAX, INT32_MAX);
        return static_cast<int32_t>(std::clamp<int64_t>(int64_t{current} + delta, -INT32_MAX, INT32_MAX));
    };
    _count_x = accumulate(_count_x, x);
    _count_y = accumulate(_count_y, -std::clamp<int64_t>(y, -INT32_MAX, INT32_MAX));
    _count_z = accumulate(_count_z, -std::clamp<int64_t>(wheel, -INT32_MAX, INT32_MAX));
}
void PS2Mouse::apply_input(int64_t x, int64_t y, int64_t wheel, uint8_t buttons)
{
    RecursiveLock lock(_mutex_state);
    move(x, y, wheel);
    const uint8_t previous = _button_left | (_button_right << 1) | (_button_middle << 2) | (_button_4th << 3) | (_button_5th << 4);
    if (previous != (buttons & 0x1F)) {
        _buttons_dirty = true;
        if (stream_reporting_enabled()) {
            if (_button_event_count == _button_events.size()) {
                _button_events.back() = buttons & 0x1F; // Recover the latest state after explicit overload.
                ESP_LOGW(TAG, "Mouse button queue overloaded; retaining latest state");
            } else _button_events[_button_event_count++] = buttons & 0x1F;
        }
    }
    _button_left = buttons & 1; _button_right = (buttons >> 1) & 1;
    _button_middle = (buttons >> 2) & 1; _button_4th = (buttons >> 3) & 1;
    _button_5th = (buttons >> 4) & 1;
}
void PS2Mouse::press(Button button)
{
    RecursiveLock lock(_mutex_state);
    if (static_cast<unsigned>(button) > 4) return;
    const uint8_t buttons = _button_left | (_button_right << 1) | (_button_middle << 2) | (_button_4th << 3) | (_button_5th << 4);
    apply_input(0, 0, 0, buttons | (1u << static_cast<unsigned>(button)));
}
void PS2Mouse::release(Button button)
{
    RecursiveLock lock(_mutex_state);
    if (static_cast<unsigned>(button) > 4) return;
    const uint8_t buttons = _button_left | (_button_right << 1) | (_button_middle << 2) | (_button_4th << 3) | (_button_5th << 4);
    apply_input(0, 0, 0, buttons & ~(1u << static_cast<unsigned>(button)));
}
void PS2Mouse::click(Button button)
{
    press(button);
    delay(MOUSE_CLICK_PRESSING_DURATION_MILLIS);
    release(button);
}
bool PS2Mouse::stream_reporting_enabled()
{
    RecursiveLock lock(_mutex_state);
    return _data_reporting_enabled && _mode == Mode::STREAM_MODE;
}
void PS2Mouse::_report(bool response)
{
    RecursiveLock lock(_mutex_state);
    if (!response && (!stream_reporting_enabled() || (!_count_x && !_count_y && !_count_z && !_buttons_dirty))) return;
    const uint8_t current_buttons = _button_left | (_button_right << 1) | (_button_middle << 2) | (_button_4th << 3) | (_button_5th << 4);
    const uint8_t buttons = !response && _button_event_count ? _button_events[0] : current_buttons;
    const auto report = encode_mouse_report(_count_x, _count_y, _count_z, buttons,
        !response && _scale == Scale::TWO_ONE, _has_wheel, _has_4th_and_5th_buttons);
    if (response) { if (!send_response(report.packet)) return; }
    else if (send_packet(&report.packet) != 0) return; // Keep counters until successfully queued.
    if (response) _button_event_count = 0;
    else if (_button_event_count) {
        --_button_event_count;
        for (size_t i = 0; i < _button_event_count; ++i) _button_events[i] = _button_events[i + 1];
    }
    _buttons_dirty = _button_event_count || buttons != current_buttons;
    _count_x = _count_y = 0;
    _count_z = _has_wheel ? _count_z - report.wheel_consumed : 0;

}
void PS2Mouse::_send_status()
{
    RecursiveLock lock(_mutex_state);
    PS2Packet packet;
    packet.len = 3;
    bool mode = (_mode == Mode::REMOTE_MODE);
    packet.data[0] = (_button_right & 1) | ((_button_middle & 1) << 1) | ((_button_left & 1) << 2) | ((0) << 3) |
                     (((uint8_t)_scale & 1) << 4) | ((_data_reporting_enabled & 1) << 5) | ((mode & 1) << 6) | ((0) << 7);
    packet.data[1] = (uint8_t)_resolution;
    packet.data[2] = _sample_rate;
    send_response(packet);
    reset_counter();
}

void _taskfn_poll_mouse_count(void *arg)
{
    auto *mouse = static_cast<PS2Mouse *>(arg);
    for (;;) {
        if (mouse->stream_reporting_enabled()) mouse->_report();
        delay(1000 / mouse->get_sample_rate());
    }
}

} // namespace esp32_ps2dev
