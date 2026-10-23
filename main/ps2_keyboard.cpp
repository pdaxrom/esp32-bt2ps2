#include "esp32-ps2dev.h"
#include <array>
#include <algorithm>
#include "ps2_timing.hpp"

#define NOP() asm volatile("nop")
#define HIGH 0x1
#define LOW 0x0

// Unomment following line to enable debug messages on the PS2DEV module
//#define _ESP32_PS2DEV_DEBUG_
#define TAG __FILE__

namespace esp32_ps2dev
{

namespace scancodes
{
#include "scan_codes_set_2.h"
#include "scan_codes_set_3.h"
static_assert(std::size(MAKE_CODES) == KEY_COUNT && std::size(MAKE_CODES_LEN) == KEY_COUNT);
static_assert(std::size(BREAK_CODES) == KEY_COUNT && std::size(BREAK_CODES_LEN) == KEY_COUNT);
static_assert(std::size(MAKE_SET3_CODES) == KEY_COUNT && std::size(MAKE_SET3_CODES_LEN) == KEY_COUNT);
static_assert(std::size(BREAK_SET3_CODES) == KEY_COUNT && std::size(BREAK_SET3_CODES_LEN) == KEY_COUNT);
}

namespace
{
constexpr int16_t INVALID_KEY = -1;

constexpr void assign_key(std::array<int16_t, 256> &map, uint8_t hid, scancodes::Key key)
{
    map[hid] = static_cast<int16_t>(key);
}

constexpr std::array<int16_t, 256> build_hid_lookup()
{
    std::array<int16_t, 256> map{};
    for (auto &entry : map) {
        entry = INVALID_KEY;
    }

    assign_key(map, 0x04, scancodes::Key::K_A);
    assign_key(map, 0x05, scancodes::Key::K_B);
    assign_key(map, 0x06, scancodes::Key::K_C);
    assign_key(map, 0x07, scancodes::Key::K_D);
    assign_key(map, 0x08, scancodes::Key::K_E);
    assign_key(map, 0x09, scancodes::Key::K_F);
    assign_key(map, 0x0A, scancodes::Key::K_G);
    assign_key(map, 0x0B, scancodes::Key::K_H);
    assign_key(map, 0x0C, scancodes::Key::K_I);
    assign_key(map, 0x0D, scancodes::Key::K_J);
    assign_key(map, 0x0E, scancodes::Key::K_K);
    assign_key(map, 0x0F, scancodes::Key::K_L);
    assign_key(map, 0x10, scancodes::Key::K_M);
    assign_key(map, 0x11, scancodes::Key::K_N);
    assign_key(map, 0x12, scancodes::Key::K_O);
    assign_key(map, 0x13, scancodes::Key::K_P);
    assign_key(map, 0x14, scancodes::Key::K_Q);
    assign_key(map, 0x15, scancodes::Key::K_R);
    assign_key(map, 0x16, scancodes::Key::K_S);
    assign_key(map, 0x17, scancodes::Key::K_T);
    assign_key(map, 0x18, scancodes::Key::K_U);
    assign_key(map, 0x19, scancodes::Key::K_V);
    assign_key(map, 0x1A, scancodes::Key::K_W);
    assign_key(map, 0x1B, scancodes::Key::K_X);
    assign_key(map, 0x1C, scancodes::Key::K_Y);
    assign_key(map, 0x1D, scancodes::Key::K_Z);
    assign_key(map, 0x1E, scancodes::Key::K_1);
    assign_key(map, 0x1F, scancodes::Key::K_2);
    assign_key(map, 0x20, scancodes::Key::K_3);
    assign_key(map, 0x21, scancodes::Key::K_4);
    assign_key(map, 0x22, scancodes::Key::K_5);
    assign_key(map, 0x23, scancodes::Key::K_6);
    assign_key(map, 0x24, scancodes::Key::K_7);
    assign_key(map, 0x25, scancodes::Key::K_8);
    assign_key(map, 0x26, scancodes::Key::K_9);
    assign_key(map, 0x27, scancodes::Key::K_0);
    assign_key(map, 0x28, scancodes::Key::K_RETURN);
    assign_key(map, 0x29, scancodes::Key::K_ESCAPE);
    assign_key(map, 0x2A, scancodes::Key::K_BACKSPACE);
    assign_key(map, 0x2B, scancodes::Key::K_TAB);
    assign_key(map, 0x2C, scancodes::Key::K_SPACE);
    assign_key(map, 0x2D, scancodes::Key::K_MINUS);
    assign_key(map, 0x2E, scancodes::Key::K_EQUALS);
    assign_key(map, 0x2F, scancodes::Key::K_LEFTBRACKET);
    assign_key(map, 0x30, scancodes::Key::K_RIGHTBRACKET);
    assign_key(map, 0x31, scancodes::Key::K_BACKSLASH);
    assign_key(map, 0x32, scancodes::Key::K_BACKSLASH);
    assign_key(map, 0x33, scancodes::Key::K_SEMICOLON);
    assign_key(map, 0x34, scancodes::Key::K_QUOTE);
    assign_key(map, 0x35, scancodes::Key::K_BACKQUOTE);
    assign_key(map, 0x36, scancodes::Key::K_COMMA);
    assign_key(map, 0x37, scancodes::Key::K_PERIOD);
    assign_key(map, 0x38, scancodes::Key::K_SLASH);
    assign_key(map, 0x39, scancodes::Key::K_CAPSLOCK);
    assign_key(map, 0x3A, scancodes::Key::K_F1);
    assign_key(map, 0x3B, scancodes::Key::K_F2);
    assign_key(map, 0x3C, scancodes::Key::K_F3);
    assign_key(map, 0x3D, scancodes::Key::K_F4);
    assign_key(map, 0x3E, scancodes::Key::K_F5);
    assign_key(map, 0x3F, scancodes::Key::K_F6);
    assign_key(map, 0x40, scancodes::Key::K_F7);
    assign_key(map, 0x41, scancodes::Key::K_F8);
    assign_key(map, 0x42, scancodes::Key::K_F9);
    assign_key(map, 0x43, scancodes::Key::K_F10);
    assign_key(map, 0x44, scancodes::Key::K_F11);
    assign_key(map, 0x45, scancodes::Key::K_F12);
    assign_key(map, 0x46, scancodes::Key::K_PRINT);
    assign_key(map, 0x47, scancodes::Key::K_SCROLLOCK);
    assign_key(map, 0x48, scancodes::Key::K_PAUSE);
    assign_key(map, 0x49, scancodes::Key::K_INSERT);
    assign_key(map, 0x4A, scancodes::Key::K_HOME);
    assign_key(map, 0x4B, scancodes::Key::K_PAGEUP);
    assign_key(map, 0x4C, scancodes::Key::K_DELETE);
    assign_key(map, 0x4D, scancodes::Key::K_END);
    assign_key(map, 0x4E, scancodes::Key::K_PAGEDOWN);
    assign_key(map, 0x4F, scancodes::Key::K_RIGHT);
    assign_key(map, 0x50, scancodes::Key::K_LEFT);
    assign_key(map, 0x51, scancodes::Key::K_DOWN);
    assign_key(map, 0x52, scancodes::Key::K_UP);
    assign_key(map, 0x53, scancodes::Key::K_NUMLOCK);
    assign_key(map, 0x54, scancodes::Key::K_KP_DIVIDE);
    assign_key(map, 0x55, scancodes::Key::K_KP_MULTIPLY);
    assign_key(map, 0x56, scancodes::Key::K_KP_MINUS);
    assign_key(map, 0x57, scancodes::Key::K_KP_PLUS);
    assign_key(map, 0x58, scancodes::Key::K_KP_ENTER);
    assign_key(map, 0x59, scancodes::Key::K_KP1);
    assign_key(map, 0x5A, scancodes::Key::K_KP2);
    assign_key(map, 0x5B, scancodes::Key::K_KP3);
    assign_key(map, 0x5C, scancodes::Key::K_KP4);
    assign_key(map, 0x5D, scancodes::Key::K_KP5);
    assign_key(map, 0x5E, scancodes::Key::K_KP6);
    assign_key(map, 0x5F, scancodes::Key::K_KP7);
    assign_key(map, 0x60, scancodes::Key::K_KP8);
    assign_key(map, 0x61, scancodes::Key::K_KP9);
    assign_key(map, 0x62, scancodes::Key::K_KP0);
    assign_key(map, 0x63, scancodes::Key::K_KP_PERIOD);
    assign_key(map, 0x64, scancodes::Key::K_ISO_BACKSLASH);
    assign_key(map, 0x65, scancodes::Key::K_MENU);
    assign_key(map, 0x66, scancodes::Key::K_ACPI_POWER);
    assign_key(map, 0x74, scancodes::Key::K_MEDIA_PLAY_PAUSE);
    assign_key(map, 0x78, scancodes::Key::K_MEDIA_STOP);
    assign_key(map, 0x7F, scancodes::Key::K_MEDIA_MUTE);
    assign_key(map, 0x80, scancodes::Key::K_MEDIA_VOLUME_UP);
    assign_key(map, 0x81, scancodes::Key::K_MEDIA_VOLUME_DOWN);
    assign_key(map, 0xE0, scancodes::Key::K_LCTRL);
    assign_key(map, 0xE1, scancodes::Key::K_LSHIFT);
    assign_key(map, 0xE2, scancodes::Key::K_LALT);
    assign_key(map, 0xE3, scancodes::Key::K_LSUPER);
    assign_key(map, 0xE4, scancodes::Key::K_RCTRL);
    assign_key(map, 0xE5, scancodes::Key::K_RSHIFT);
    assign_key(map, 0xE6, scancodes::Key::K_RALT);
    assign_key(map, 0xE7, scancodes::Key::K_RSUPER);

    return map;
}

constexpr std::array<int16_t, 256> HID_TO_PS2_LOOKUP = build_hid_lookup();

struct ConsumerMapping {
    uint16_t usage;
    scancodes::Key key;
};

constexpr ConsumerMapping CONSUMER_MAPPINGS[] = {
    {0xCD, scancodes::Key::K_MEDIA_PLAY_PAUSE},
    {0xE9, scancodes::Key::K_MEDIA_VOLUME_UP},
    {0xEA, scancodes::Key::K_MEDIA_VOLUME_DOWN},
    {0xB6, scancodes::Key::K_MEDIA_PREV_TRACK},
    {0xB5, scancodes::Key::K_MEDIA_NEXT_TRACK},
    {0x183, scancodes::Key::K_MEDIA_MEDIA_SELECT},
    {0x18A, scancodes::Key::K_MEDIA_EMAIL},
    {0xE2, scancodes::Key::K_MEDIA_MUTE},
    {0x221, scancodes::Key::K_MEDIA_WWW_SEARCH},
    {0x223, scancodes::Key::K_HOME},
    {0x196, scancodes::Key::K_MEDIA_WWW_HOME},
    {0x224, scancodes::Key::K_MEDIA_WWW_BACK},
};

bool map_consumer_usage(uint16_t usage, scancodes::Key &out_key)
{
    for (const auto &entry : CONSUMER_MAPPINGS) {
        if (entry.usage == usage) {
            out_key = entry.key;
            return true;
        }
    }
    return false;
}

bool key_from_set3_code(uint8_t code, scancodes::Key &out_key)
{
    for (size_t idx = 0; idx < scancodes::KEY_COUNT; ++idx) {
        if (scancodes::MAKE_SET3_CODES_LEN[idx] == 1 && scancodes::MAKE_SET3_CODES[idx][0] == code) {
            out_key = static_cast<scancodes::Key>(idx);
            return true;
        }
    }
    return false;
}

using KeyBehavior = PS2Keyboard::KeyBehavior;
constexpr KeyBehavior BEHAVIOR_TYP_ONLY{true, false, true};
constexpr KeyBehavior BEHAVIOR_MAKE_BREAK{true, true, false};
constexpr KeyBehavior BEHAVIOR_MAKE_ONLY{true, false, false};
constexpr KeyBehavior BEHAVIOR_TYP_MAKE_BREAK{true, true, true};

// PC_AT_Technical_Reference_Mar86.pdf
// page 4-58
constexpr std::array SET3_DEFAULT_MAKE_BREAK_CODES = {
    0x14, 0x12, 0x59, 0x11, 0x19,
};
constexpr std::array SET3_DEFAULT_MAKE_ONLY_CODES = {
    0x39, 0x58, 0x67, 0x6E, 0x65, 0x6F, 0x6D, 0x76,
    0x6C, 0x6B, 0x69, 0x77, 0x75, 0x73, 0x72, 0x70,
    0x7E, 0x74, 0x7A, 0x71, 0x84, 0x79, 0x08, 0x07,
    0x0F, 0x17, 0x1F, 0x27, 0x2F, 0x37, 0x3F, 0x47,
    0x4F, 0x56, 0x5E, 0x57, 0x5F, 0x62,
};

// PS/2 rate = 240 / ((8 + A) * 2^B) Hz; round the period to milliseconds.
constexpr uint16_t typematic_cycle(uint8_t code)
{
    const uint32_t units = (8 + (code & 7)) << ((code >> 3) & 3);
    return (units * 1000 + 120) / 240;
}
constexpr uint16_t DELAY_TABLE[4] = {250, 500, 750, 1000};
} // namespace

PS2Keyboard::PS2Keyboard(int clk, int data) : PS2dev(clk, data) {}
uint8_t PS2Keyboard::canonical_hid_key(uint8_t code)
{
    const auto mapped = HID_TO_PS2_LOOKUP[code];
    if (mapped == INVALID_KEY) return code;
    for (size_t key = 1; key < HID_TO_PS2_LOOKUP.size(); ++key)
        if (HID_TO_PS2_LOOKUP[key] == mapped) return key;
    return code;
}
uint8_t PS2Keyboard::consumer_hid_key(uint16_t code)
{
    scancodes::Key mapped;
    if (!map_consumer_usage(code, mapped)) return 0;
    for (size_t key = 1; key < HID_TO_PS2_LOOKUP.size(); ++key)
        if (HID_TO_PS2_LOOKUP[key] == static_cast<int16_t>(mapped)) return key;
    return 0;
}
void PS2Keyboard::begin()
{
    PS2dev::begin(_config_task_core);
    apply_default_key_behaviors();
    delay(200);
    send_byte(0xAA);
    start_bus_task();
}
void PS2Keyboard::apply_behavior_to_all(const KeyBehavior &behavior)
{
    RecursiveLock lock(_mutex_state);
    for (auto &entry : _key_behaviors) {
        entry = behavior;
    }
}
void PS2Keyboard::apply_default_key_behaviors()
{
    RecursiveLock lock(_mutex_state);
    update_typematic(0x2B);
    apply_behavior_to_all(BEHAVIOR_TYP_MAKE_BREAK);
    if (_scan_code_set != 3) {
        return;
    }
    const auto apply_for_codes = [this](const auto &codes, const KeyBehavior &behavior) {
        for (auto code : codes) {
            configure_specific_key(code, behavior);
        }
    };
    apply_for_codes(SET3_DEFAULT_MAKE_BREAK_CODES, BEHAVIOR_MAKE_BREAK);
    apply_for_codes(SET3_DEFAULT_MAKE_ONLY_CODES, BEHAVIOR_MAKE_ONLY);
}
void PS2Keyboard::set_key_behavior(scancodes::Key key, const KeyBehavior &behavior)
{
    RecursiveLock lock(_mutex_state);
    _key_behaviors[static_cast<size_t>(key)] = behavior;
}
const PS2Keyboard::KeyBehavior &PS2Keyboard::behavior_for_key(scancodes::Key key) const
{
    return _key_behaviors[static_cast<size_t>(key)];
}
bool PS2Keyboard::allows_typematic_for_key(scancodes::Key key) const
{
    return behavior_for_key(key).typematic;
}
bool PS2Keyboard::allows_typematic(uint8_t hid_code) const
{
    RecursiveLock lock(_mutex_state);
    int16_t mapped = HID_TO_PS2_LOOKUP[hid_code];
    if (mapped == INVALID_KEY) {
        return false;
    }
    return _key_behaviors[static_cast<size_t>(mapped)].typematic;
}

void PS2Keyboard::update_typematic(uint8_t config)
{
    RecursiveLock lock(_mutex_state);
#if defined(_ESP32_PS2DEV_DEBUG_)
    ESP_LOGI(TAG, "typematic config = %02X", config);
#endif
    _typematic_config = config;
    uint8_t delay_index = (config >> 5) & 0x03;
    if (delay_index > 3) {
        delay_index = 3;
    }
    _typematic_delay_ms = DELAY_TABLE[delay_index];
    uint8_t rate_index = config & 0x1F;
    _typematic_cycle_ms = typematic_cycle(rate_index);

#if defined(_ESP32_PS2DEV_DEBUG_)
    ESP_LOGI(TAG, "typematic delay ms = %d, cycle ms = %d", _typematic_delay_ms, _typematic_cycle_ms);
#endif
}
void PS2Keyboard::configure_specific_key(uint8_t scan_code, const KeyBehavior &behavior)
{
    RecursiveLock lock(_mutex_state);
    if (_scan_code_set != 3) {
        return;
    }
    scancodes::Key key;
    if (key_from_set3_code(scan_code, key)) {
        set_key_behavior(key, behavior);
    } else {
        ESP_LOGW(TAG, "Unknown Set 3 key code 0x%02x for make-only configuration", scan_code);
    }
}
bool PS2Keyboard::data_reporting_enabled()
{
    RecursiveLock lock(_mutex_state);
    return _data_reporting_enabled;
}
bool PS2Keyboard::is_scroll_lock_led_on()
{
    RecursiveLock lock(_mutex_state);
    return _led_scroll_lock;
}
bool PS2Keyboard::is_num_lock_led_on()
{
    RecursiveLock lock(_mutex_state);
    return _led_num_lock;
}
bool PS2Keyboard::is_caps_lock_led_on()
{
    RecursiveLock lock(_mutex_state);
    return _led_caps_lock;
}
int PS2Keyboard::reply_to_host(uint8_t value)
{
    RecursiveLock lock(_mutex_state);
    // Parameters arrive as later host requests. Do not wait with the bus/state locked.
    if (_pending_command && value < 0xED) {
        const auto command = static_cast<Command>(_pending_command);
        switch (command) {
        case Command::SET_TYPEMATIC_RATE:
            if (value > 0x7F) { send_byte(0xFE); return 0; }
            update_typematic(value); ack(); break;
        case Command::SET_SCAN_CODE_SET:
            if (value != 0 && value != 2 && value != 3) { send_byte(0xFE); return 0; }
            if (value) {
                discard_queued_packets();
                _scan_code_set = value;
                apply_default_key_behaviors();
            }
            ack();
            if (!value) send_byte(static_cast<uint8_t>(_scan_code_set));
            break;
        case Command::SET_RESET_LEDS:
            if (value > 7) { send_byte(0xFE); return 0; }
            _led_scroll_lock = value & KeyLed::KEYBOARD_LED_SCROLLLOCK;
            _led_num_lock = value & KeyLed::KEYBOARD_LED_NUMLOCK;
            _led_caps_lock = value & KeyLed::KEYBOARD_LED_CAPSLOCK;
            ack(); trigger_leds_callback(value); break;
        default: {
            scancodes::Key key;
            if (!key_from_set3_code(value, key)) { send_byte(0xFE); return 0; }
            const auto behavior = command == Command::SET_SPECIFIC_KEY_TO_MAKE_ONLY ? BEHAVIOR_MAKE_ONLY
                : command == Command::SET_SPECIFIC_KEY_TO_MAKE_RELEASE ? BEHAVIOR_MAKE_BREAK : BEHAVIOR_TYP_ONLY;
            configure_specific_key(value, behavior); ack();
            return 0; // Set 3 commands accept a list until the next command.
        }
        }
        _pending_command = 0;
        return 0;
    }
    _pending_command = 0;
    switch (static_cast<Command>(value)) {
    case Command::RESET:
        discard_queued_packets();
        _data_reporting_enabled = true;
        _scan_code_set = 2;
        apply_default_key_behaviors();
        _led_scroll_lock = _led_num_lock = _led_caps_lock = false;
        trigger_leds_callback(0);
        ack(); send_byte(static_cast<uint8_t>(Command::BAT_SUCCESS)); break;
    case Command::RESEND: resend_byte(); break;
    case Command::SET_DEFAULTS:
        discard_queued_packets();
        _scan_code_set = 2;
        _data_reporting_enabled = true;
        apply_default_key_behaviors(); ack(); break;
    case Command::DISABLE_DATA_REPORTING:
        discard_queued_packets();
        _scan_code_set = 2;
        apply_default_key_behaviors();
        _data_reporting_enabled = false;
        ack(); break;
    case Command::ENABLE_DATA_REPORTING: _data_reporting_enabled = true; ack(); break;
    case Command::SET_TYPEMATIC_RATE:
    case Command::SET_SCAN_CODE_SET:
    case Command::SET_RESET_LEDS:
        _pending_command = value; ack(); break;
    case Command::GET_DEVICE_ID: {
        ack();
        PS2Packet id{}; id.len = 2; id.data[0] = 0xAB; id.data[1] = 0x83;
        send_response(id); break;
    }
    case Command::ECHO: send_byte(0xEE); break;
    case Command::SET_ALL_KEYS_TO_TYPEMATIC_AUTOREPEAT_ONLY:
    case Command::SET_ALL_KEYS_TO_MAKE_RELEASE:
    case Command::SET_ALL_KEYS_TO_MAKE_ONLY:
    case Command::SET_ALL_KEYS_TO_TYPEMATIC_AUTOREPEAT_MAKE_RELEASE:
        if (_scan_code_set == 3) {
            const auto command = static_cast<Command>(value);
            const auto behavior = command == Command::SET_ALL_KEYS_TO_MAKE_ONLY ? BEHAVIOR_MAKE_ONLY
                : command == Command::SET_ALL_KEYS_TO_MAKE_RELEASE ? BEHAVIOR_MAKE_BREAK
                : command == Command::SET_ALL_KEYS_TO_TYPEMATIC_AUTOREPEAT_ONLY ? BEHAVIOR_TYP_ONLY : BEHAVIOR_TYP_MAKE_BREAK;
            apply_behavior_to_all(behavior);
        }
        ack(); break;
    case Command::SET_SPECIFIC_KEY_TO_TYPEMATIC_AUTOREPEAT_ONLY:
    case Command::SET_SPECIFIC_KEY_TO_MAKE_RELEASE:
    case Command::SET_SPECIFIC_KEY_TO_MAKE_ONLY:
        if (_scan_code_set == 3) _pending_command = value;
        ack(); break;
    default: send_byte(0xFE); break;
    }
    return 0;
}
bool PS2Keyboard::keydown(scancodes::Key key)
{
    RecursiveLock lock(_mutex_state);
    if (!_data_reporting_enabled) {
        return false;
    }
    const auto &behavior = behavior_for_key(key);
    if (!behavior.make) {
        return true;
    }
    PS2Packet packet;
    packet.len = (_scan_code_set == 3) ? scancodes::MAKE_SET3_CODES_LEN[key] : scancodes::MAKE_CODES_LEN[key];
    for (uint8_t i = 0; i < packet.len; i++) {
        packet.data[i] = (_scan_code_set == 3) ? scancodes::MAKE_SET3_CODES[key][i] : scancodes::MAKE_CODES[key][i];
    }
    return send_packet(&packet) == 0;
}
bool PS2Keyboard::keyup(scancodes::Key key)
{
    RecursiveLock lock(_mutex_state);
    if (!_data_reporting_enabled) {
        return false;
    }
    const auto &behavior = behavior_for_key(key);
    if (!behavior.break_code) {
        return true;
    }
    PS2Packet packet;
    packet.len = (_scan_code_set == 3) ? scancodes::BREAK_SET3_CODES_LEN[key] : scancodes::BREAK_CODES_LEN[key];
    for (uint8_t i = 0; i < packet.len; i++) {
        packet.data[i] = (_scan_code_set == 3) ? scancodes::BREAK_SET3_CODES[key][i] : scancodes::BREAK_CODES[key][i];
    }
    return send_packet(&packet) == 0;
}
void PS2Keyboard::type(scancodes::Key key)
{
    keydown(key);
    delay(10);
    keyup(key);
}
void PS2Keyboard::type(std::initializer_list<scancodes::Key> keys)
{
    std::stack<scancodes::Key> stack;
    for (auto key : keys) {
        keydown(key);
        stack.push(key);
        delay(10);
    }
    while (!stack.empty()) {
        keyup(stack.top());
        stack.pop();
        delay(10);
    }
}
void PS2Keyboard::type(const char *text)
{
    struct AsciiKey { scancodes::Key key = static_cast<scancodes::Key>(scancodes::KEY_COUNT); bool shift = false; };
    static constexpr auto keys = [] {
        std::array<AsciiKey, 128> result{};
        result[static_cast<unsigned char>('\b')] = {scancodes::Key::K_BACKSPACE, false};
        result[static_cast<unsigned char>('\t')] = {scancodes::Key::K_TAB, false};
        result[static_cast<unsigned char>('\r')] = {scancodes::Key::K_RETURN, false};
        result[static_cast<unsigned char>('\n')] = {scancodes::Key::K_RETURN, false};
        result[static_cast<unsigned char>(' ')] = {scancodes::Key::K_SPACE, false};
        result[static_cast<unsigned char>('!')] = {scancodes::Key::K_1, true};
        result[static_cast<unsigned char>('\"')] = {scancodes::Key::K_QUOTE, true};
        result[static_cast<unsigned char>('#')] = {scancodes::Key::K_3, true};
        result[static_cast<unsigned char>('$')] = {scancodes::Key::K_4, true};
        result[static_cast<unsigned char>('&')] = {scancodes::Key::K_7, true};
        result[static_cast<unsigned char>('\'')] = {scancodes::Key::K_QUOTE, false};
        result[static_cast<unsigned char>('(')] = {scancodes::Key::K_9, true};
        result[static_cast<unsigned char>(')')] = {scancodes::Key::K_0, true};
        result[static_cast<unsigned char>('*')] = {scancodes::Key::K_8, true};
        result[static_cast<unsigned char>('+')] = {scancodes::Key::K_EQUALS, true};
        result[static_cast<unsigned char>(',')] = {scancodes::Key::K_COMMA, false};
        result[static_cast<unsigned char>('-')] = {scancodes::Key::K_MINUS, false};
        result[static_cast<unsigned char>('.')] = {scancodes::Key::K_PERIOD, false};
        result[static_cast<unsigned char>('/')] = {scancodes::Key::K_SLASH, false};
        result[static_cast<unsigned char>('0')] = {scancodes::Key::K_0, false};
        result[static_cast<unsigned char>('1')] = {scancodes::Key::K_1, false};
        result[static_cast<unsigned char>('2')] = {scancodes::Key::K_2, false};
        result[static_cast<unsigned char>('3')] = {scancodes::Key::K_3, false};
        result[static_cast<unsigned char>('4')] = {scancodes::Key::K_4, false};
        result[static_cast<unsigned char>('5')] = {scancodes::Key::K_5, false};
        result[static_cast<unsigned char>('6')] = {scancodes::Key::K_6, false};
        result[static_cast<unsigned char>('7')] = {scancodes::Key::K_7, false};
        result[static_cast<unsigned char>('8')] = {scancodes::Key::K_8, false};
        result[static_cast<unsigned char>('9')] = {scancodes::Key::K_9, false};
        result[static_cast<unsigned char>(':')] = {scancodes::Key::K_SEMICOLON, true};
        result[static_cast<unsigned char>(';')] = {scancodes::Key::K_SEMICOLON, false};
        result[static_cast<unsigned char>('<')] = {scancodes::Key::K_COMMA, true};
        result[static_cast<unsigned char>('=')] = {scancodes::Key::K_EQUALS, false};
        result[static_cast<unsigned char>('>')] = {scancodes::Key::K_PERIOD, true};
        result[static_cast<unsigned char>('\?')] = {scancodes::Key::K_SLASH, true};
        result[static_cast<unsigned char>('@')] = {scancodes::Key::K_2, true};
        result[static_cast<unsigned char>('[')] = {scancodes::Key::K_LEFTBRACKET, false};
        result[static_cast<unsigned char>('\\')] = {scancodes::Key::K_BACKSLASH, false};
        result[static_cast<unsigned char>(']')] = {scancodes::Key::K_RIGHTBRACKET, false};
        result[static_cast<unsigned char>('^')] = {scancodes::Key::K_6, true};
        result[static_cast<unsigned char>('_')] = {scancodes::Key::K_MINUS, true};
        result[static_cast<unsigned char>('`')] = {scancodes::Key::K_BACKQUOTE, false};
        result[static_cast<unsigned char>('a')] = {scancodes::Key::K_A, false};
        result[static_cast<unsigned char>('b')] = {scancodes::Key::K_B, false};
        result[static_cast<unsigned char>('c')] = {scancodes::Key::K_C, false};
        result[static_cast<unsigned char>('d')] = {scancodes::Key::K_D, false};
        result[static_cast<unsigned char>('e')] = {scancodes::Key::K_E, false};
        result[static_cast<unsigned char>('f')] = {scancodes::Key::K_F, false};
        result[static_cast<unsigned char>('g')] = {scancodes::Key::K_G, false};
        result[static_cast<unsigned char>('h')] = {scancodes::Key::K_H, false};
        result[static_cast<unsigned char>('i')] = {scancodes::Key::K_I, false};
        result[static_cast<unsigned char>('j')] = {scancodes::Key::K_J, false};
        result[static_cast<unsigned char>('k')] = {scancodes::Key::K_K, false};
        result[static_cast<unsigned char>('l')] = {scancodes::Key::K_L, false};
        result[static_cast<unsigned char>('m')] = {scancodes::Key::K_M, false};
        result[static_cast<unsigned char>('n')] = {scancodes::Key::K_N, false};
        result[static_cast<unsigned char>('o')] = {scancodes::Key::K_O, false};
        result[static_cast<unsigned char>('p')] = {scancodes::Key::K_P, false};
        result[static_cast<unsigned char>('q')] = {scancodes::Key::K_Q, false};
        result[static_cast<unsigned char>('r')] = {scancodes::Key::K_R, false};
        result[static_cast<unsigned char>('s')] = {scancodes::Key::K_S, false};
        result[static_cast<unsigned char>('t')] = {scancodes::Key::K_T, false};
        result[static_cast<unsigned char>('u')] = {scancodes::Key::K_U, false};
        result[static_cast<unsigned char>('v')] = {scancodes::Key::K_V, false};
        result[static_cast<unsigned char>('w')] = {scancodes::Key::K_W, false};
        result[static_cast<unsigned char>('x')] = {scancodes::Key::K_X, false};
        result[static_cast<unsigned char>('y')] = {scancodes::Key::K_Y, false};
        result[static_cast<unsigned char>('z')] = {scancodes::Key::K_Z, false};
        result[static_cast<unsigned char>('A')] = {scancodes::Key::K_A, true};
        result[static_cast<unsigned char>('B')] = {scancodes::Key::K_B, true};
        result[static_cast<unsigned char>('C')] = {scancodes::Key::K_C, true};
        result[static_cast<unsigned char>('D')] = {scancodes::Key::K_D, true};
        result[static_cast<unsigned char>('E')] = {scancodes::Key::K_E, true};
        result[static_cast<unsigned char>('F')] = {scancodes::Key::K_F, true};
        result[static_cast<unsigned char>('G')] = {scancodes::Key::K_G, true};
        result[static_cast<unsigned char>('H')] = {scancodes::Key::K_H, true};
        result[static_cast<unsigned char>('I')] = {scancodes::Key::K_I, true};
        result[static_cast<unsigned char>('J')] = {scancodes::Key::K_J, true};
        result[static_cast<unsigned char>('K')] = {scancodes::Key::K_K, true};
        result[static_cast<unsigned char>('L')] = {scancodes::Key::K_L, true};
        result[static_cast<unsigned char>('M')] = {scancodes::Key::K_M, true};
        result[static_cast<unsigned char>('N')] = {scancodes::Key::K_N, true};
        result[static_cast<unsigned char>('O')] = {scancodes::Key::K_O, true};
        result[static_cast<unsigned char>('P')] = {scancodes::Key::K_P, true};
        result[static_cast<unsigned char>('Q')] = {scancodes::Key::K_Q, true};
        result[static_cast<unsigned char>('R')] = {scancodes::Key::K_R, true};
        result[static_cast<unsigned char>('S')] = {scancodes::Key::K_S, true};
        result[static_cast<unsigned char>('T')] = {scancodes::Key::K_T, true};
        result[static_cast<unsigned char>('U')] = {scancodes::Key::K_U, true};
        result[static_cast<unsigned char>('V')] = {scancodes::Key::K_V, true};
        result[static_cast<unsigned char>('W')] = {scancodes::Key::K_W, true};
        result[static_cast<unsigned char>('X')] = {scancodes::Key::K_X, true};
        result[static_cast<unsigned char>('Y')] = {scancodes::Key::K_Y, true};
        result[static_cast<unsigned char>('Z')] = {scancodes::Key::K_Z, true};
        return result;
    }();
    for (; *text; ++text) {
        const auto code = static_cast<unsigned char>(*text);
        if (code >= keys.size() || keys[code].key == static_cast<scancodes::Key>(scancodes::KEY_COUNT)) continue;
        const auto entry = keys[code];
        if (entry.shift) { keydown(scancodes::Key::K_LSHIFT); delay(10); }
        type(entry.key);
        if (entry.shift) { delay(10); keyup(scancodes::Key::K_LSHIFT); }
    }
}

bool PS2Keyboard::keyHid_send(uint8_t btkey, bool keyDown)
{
    int16_t mapped = HID_TO_PS2_LOOKUP[btkey];
    if (mapped == INVALID_KEY) {
        return true;
    }

    scancodes::Key key = static_cast<scancodes::Key>(mapped);
    if (keyDown) {
        return keydown(key);
    } else {
        return keyup(key);
    }
}

bool PS2Keyboard::keyHid_send_CCONTROL(uint16_t btkey, bool keyDown)
{
    scancodes::Key key;
    if (!map_consumer_usage(btkey, key)) {
        return true;
    }

    if (keyDown) {
        return keydown(key);
    } else {
        return keyup(key);
    }
}


} // namespace esp32_ps2dev
