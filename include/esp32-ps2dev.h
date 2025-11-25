#ifndef ESP32_PS2DEV_H
#define ESP32_PS2DEV_H

#include <initializer_list>
#include <stack>
#include <array>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "scan_codes.h"
#include <nvs_flash.h>
#include <string>

namespace esp32_ps2dev
{
  const uint32_t CLK_HALF_PERIOD_MICROS = 40;
  const uint32_t CLK_QUATER_PERIOD_MICROS = CLK_HALF_PERIOD_MICROS / 2;
  const uint32_t BYTE_INTERVAL_MICROS = 500;
  const int PACKET_QUEUE_LENGTH = 20;
  const UBaseType_t DEFAULT_TASK_PRIORITY = 10;
  const BaseType_t DEFAULT_TASK_CORE = 0;
  const BaseType_t DEFAULT_TASK_CORE_MOUSE = 1;
  const uint32_t INTERVAL_CHECKING_HOST_SEND_REQUEST_MILLIS = 9;
  const uint32_t MOUSE_CLICK_PRESSING_DURATION_MILLIS = 100;

  class PS2Packet
  {
  public:
    uint8_t len;
    uint8_t data[16];
  };

  class PS2dev
  {
  public:
    PS2dev(int clk, int data);
    virtual ~PS2dev() = default;

    enum class BusState
    {
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
    int send_packet(PS2Packet *packet);

  protected:
    int _ps2clk;
    int _ps2data;
    UBaseType_t _config_task_priority = DEFAULT_TASK_PRIORITY;
    BaseType_t _config_task_core = DEFAULT_TASK_CORE;
    TaskHandle_t _task_process_host_request;
    TaskHandle_t _task_send_packet;
    QueueHandle_t _queue_packet;
    SemaphoreHandle_t _mutex_bus;
    void golo(int pin);
    void gohi(int pin);
    void ack();
  };

  class PS2Mouse : public PS2dev
  {
  public:
    PS2Mouse(int clk, int data);
    enum class ResolutionCode : uint8_t
    {
      RES_1 = 0x00,
      RES_2 = 0x01,
      RES_4 = 0x02,
      RES_8 = 0x03
    };
    enum class Scale : uint8_t
    {
      ONE_ONE = 0,
      TWO_ONE = 1
    };
    enum class Mode : uint8_t
    {
      REMOTE_MODE = 0,
      STREAM_MODE = 1,
      WRAP_MODE = 2
    };
    enum class Command : uint8_t
    {
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
    enum class Button : uint8_t
    {
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
    void move(int16_t x, int16_t y, int8_t wheel);
    void press(Button button);
    void release(Button button);
    void click(Button button);
    void _report();

  protected:
    static constexpr char const *TAG = "PS2Mouse";
    void _send_status();
    void _save_internal_state_to_nvs();
    void _load_internal_state_from_nvs();
    TaskHandle_t _task_poll_mouse_count;
    nvs_handle _nvs_handle;
    bool _has_wheel = false;
    bool _has_4th_and_5th_buttons = false;
    bool _data_reporting_enabled = false;
    ResolutionCode _resolution = ResolutionCode::RES_4;
    Scale _scale = Scale::ONE_ONE;
    Mode _mode = Mode::STREAM_MODE;
    Mode _last_mode = Mode::STREAM_MODE;
    uint8_t _last_sample_rate[3] = {0, 0, 0};
    uint8_t _sample_rate = 100;
    int16_t _count_x = 0;
    uint8_t _count_x_overflow = 0;
    int16_t _count_y = 0;
    uint8_t _count_y_overflow = 0;
    int8_t _count_z = 0;
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
    enum class Command
    {
      RESET = 0xFF,
      RESEND = 0xFE,
      ACK = 0xFA,
      SET_SPECIFIC_KEY_TO_MAKE_ONLY = 0xFD,
      SET_SPECIFIC_KEY_TO_MAKE_RELEASE = 0xFC,
      SET_SPECIFIC_KEY_TO_TYPEMATIC_AUTOREPEAT_ONLY = 0xFB,
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

    enum KeyLed : uint8_t
    {
      KEYBOARD_LED_SCROLLLOCK = 1 << 0,
      KEYBOARD_LED_NUMLOCK = 1 << 1,
      KEYBOARD_LED_CAPSLOCK = 1 << 2,
    };

    struct KeyBehavior
    {
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
    void keydown(scancodes::Key key);
    void keyup(scancodes::Key key);
    void type(scancodes::Key key);
    void type(std::initializer_list<scancodes::Key> keys);
    void type(const char *str);
    void keyHid_send(uint8_t btkey, bool keyDown);
    void keyHid_send_CCONTROL(uint16_t btkey, bool keyDown);
    bool allows_typematic(uint8_t hid_code) const;
    void configure_specific_key(uint8_t scan_code, const KeyBehavior &behavior);
    void configure_default_set3_behavior();
    uint16_t get_typematic_delay_ms() const { return _typematic_delay_ms; }
    uint16_t get_typematic_cycle_ms() const { return _typematic_cycle_ms; }
    uint8_t get_typematic_config() const { return _typematic_config; }

    void set_leds_callback(_leds_callback cb) { leds_callback = cb; }
    void trigger_leds_callback(uint8_t leds)
    {
      if (leds_callback != nullptr)
        leds_callback(leds);
    }

  protected:
    bool _data_reporting_enabled = true;
    bool _led_scroll_lock = false;
    bool _led_num_lock = false;
    bool _led_caps_lock = false;
    int _scan_code_set = 2;
    std::array<KeyBehavior, scancodes::KEY_COUNT> _key_behaviors{};
    void apply_behavior_to_all(const KeyBehavior &behavior);
    void set_key_behavior(scancodes::Key key, const KeyBehavior &behavior);
    const KeyBehavior &behavior_for_key(scancodes::Key key) const;
    bool allows_typematic_for_key(scancodes::Key key) const;
    void update_typematic(uint8_t config);
    uint8_t _typematic_config = 0x0B;
    uint16_t _typematic_delay_ms = 500;
    uint16_t _typematic_cycle_ms = 50;
    _leds_callback leds_callback = nullptr;
  };

  void _taskfn_process_host_request(void *arg);
  void _taskfn_send_packet(void *arg);
  void _taskfn_poll_mouse_count(void *arg);

} // namespace esp32_ps2dev

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
  PS2_MOUSE_BUTTON_LEFT = 0,
  PS2_MOUSE_BUTTON_RIGHT,
  PS2_MOUSE_BUTTON_MIDDLE,
  PS2_MOUSE_BUTTON_4,
  PS2_MOUSE_BUTTON_5,
} ps2_mouse_button_t;

typedef struct ps2_mouse_handle ps2_mouse_t;
typedef struct ps2_keyboard_handle ps2_keyboard_t;

ps2_mouse_t *ps2_mouse_create(int clk, int data);
void ps2_mouse_destroy(ps2_mouse_t *mouse);
void ps2_mouse_begin(ps2_mouse_t *mouse);
bool ps2_mouse_has_wheel(const ps2_mouse_t *mouse);
bool ps2_mouse_has_4th_and_5th_buttons(const ps2_mouse_t *mouse);
bool ps2_mouse_data_reporting_enabled(const ps2_mouse_t *mouse);
void ps2_mouse_reset_counter(ps2_mouse_t *mouse);
uint8_t ps2_mouse_get_sample_rate(const ps2_mouse_t *mouse);
void ps2_mouse_move(ps2_mouse_t *mouse, int16_t x, int16_t y, int8_t wheel);
void ps2_mouse_press(ps2_mouse_t *mouse, ps2_mouse_button_t button);
void ps2_mouse_release(ps2_mouse_t *mouse, ps2_mouse_button_t button);
void ps2_mouse_click(ps2_mouse_t *mouse, ps2_mouse_button_t button);

ps2_keyboard_t *ps2_keyboard_create(int clk, int data);
void ps2_keyboard_destroy(ps2_keyboard_t *keyboard);
void ps2_keyboard_begin(ps2_keyboard_t *keyboard);
bool ps2_keyboard_data_reporting_enabled(const ps2_keyboard_t *keyboard);
bool ps2_keyboard_is_scroll_lock_on(const ps2_keyboard_t *keyboard);
bool ps2_keyboard_is_num_lock_on(const ps2_keyboard_t *keyboard);
bool ps2_keyboard_is_caps_lock_on(const ps2_keyboard_t *keyboard);
void ps2_keyboard_keydown(ps2_keyboard_t *keyboard, esp32_ps2dev_key_t key);
void ps2_keyboard_keyup(ps2_keyboard_t *keyboard, esp32_ps2dev_key_t key);
void ps2_keyboard_type_key(ps2_keyboard_t *keyboard, esp32_ps2dev_key_t key);
void ps2_keyboard_type_text(ps2_keyboard_t *keyboard, const char *text);
void ps2_keyboard_send_hid_key(ps2_keyboard_t *keyboard, uint8_t bt_key, bool key_down);
void ps2_keyboard_send_consumer_key(ps2_keyboard_t *keyboard, uint16_t usage, bool key_down);
bool ps2_keyboard_allows_typematic(const ps2_keyboard_t *keyboard, uint8_t hid_code);
uint16_t ps2_keyboard_get_typematic_delay_ms(const ps2_keyboard_t *keyboard);
uint16_t ps2_keyboard_get_typematic_cycle_ms(const ps2_keyboard_t *keyboard);
uint8_t ps2_keyboard_get_typematic_config(const ps2_keyboard_t *keyboard);
void ps2_keyboard_set_leds_callback(ps2_keyboard_t *keyboard, void (*cb)(uint8_t leds));
void ps2_keyboard_trigger_leds_callback(ps2_keyboard_t *keyboard, uint8_t leds);

#ifdef __cplusplus
}
#endif

#endif /* ESP32_PS2DEV_H */
