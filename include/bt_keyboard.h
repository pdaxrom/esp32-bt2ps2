#ifndef BT_KEYBOARD_H
#define BT_KEYBOARD_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/event_groups.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_bt.h"
#include "esp_bt_defs.h"
#include "esp_bt_device.h"
#include "esp_bt_main.h"
#include "esp_hidh.h"
#include "esp_hid_common.h"
#include "esp_gap_bt_api.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_wifi.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BT_KEYBOARD_MAX_TEMP_USAGE_VALUES 64
#define BT_KEYBOARD_MAX_MULTIMEDIA_KEYS 16
#define BT_KEYBOARD_MAX_MULTIMEDIA_REPORTS 16
#define BT_KEYBOARD_MAX_MULTIMEDIA_USAGES 32
#define BT_KEYBOARD_MAX_MOUSE_REPORTS 16
#define BT_KEYBOARD_MAX_LED_DEVICES 8
#define BT_KEYBOARD_MAX_KEY_COUNT 10

#define BT_KEYBOARD_KEY_CAPS_LOCK 0x39

typedef void (*bt_keyboard_pid_handler_t)(uint32_t code);

typedef enum {
    BT_KEYBOARD_KEY_MODIFIER_L_CTRL = 0x01,
    BT_KEYBOARD_KEY_MODIFIER_L_SHIFT = 0x02,
    BT_KEYBOARD_KEY_MODIFIER_L_ALT = 0x04,
    BT_KEYBOARD_KEY_MODIFIER_L_META = 0x08,
    BT_KEYBOARD_KEY_MODIFIER_R_CTRL = 0x10,
    BT_KEYBOARD_KEY_MODIFIER_R_SHIFT = 0x20,
    BT_KEYBOARD_KEY_MODIFIER_R_ALT = 0x40,
    BT_KEYBOARD_KEY_MODIFIER_R_META = 0x80
} bt_keyboard_key_modifier_t;

#define BT_KEYBOARD_CTRL_MASK  ((uint8_t)(BT_KEYBOARD_KEY_MODIFIER_L_CTRL | BT_KEYBOARD_KEY_MODIFIER_R_CTRL))
#define BT_KEYBOARD_SHIFT_MASK ((uint8_t)(BT_KEYBOARD_KEY_MODIFIER_L_SHIFT | BT_KEYBOARD_KEY_MODIFIER_R_SHIFT))
#define BT_KEYBOARD_ALT_MASK   ((uint8_t)(BT_KEYBOARD_KEY_MODIFIER_L_ALT | BT_KEYBOARD_KEY_MODIFIER_R_ALT))
#define BT_KEYBOARD_META_MASK  ((uint8_t)(BT_KEYBOARD_KEY_MODIFIER_L_META | BT_KEYBOARD_KEY_MODIFIER_R_META))

typedef enum {
    BT_KEYBOARD_LED_NUMLOCK  = 1 << 0,
    BT_KEYBOARD_LED_CAPSLOCK = 1 << 1,
    BT_KEYBOARD_LED_SCROLLLOCK = 1 << 2,
    BT_KEYBOARD_LED_COMPOSE  = 1 << 3,
    BT_KEYBOARD_LED_KANA     = 1 << 4
} bt_keyboard_key_led_t;

typedef struct {
    bt_keyboard_key_modifier_t modifier;
    uint8_t keys[BT_KEYBOARD_MAX_KEY_COUNT];
} bt_keyboard_key_info_t;

typedef struct {
    uint16_t keys[BT_KEYBOARD_MAX_KEY_COUNT];
} bt_keyboard_key_info_ccontrol_t;

typedef struct {
    int16_t mouse_x;
    int16_t mouse_y;
    int8_t mouse_w;
    uint8_t mouse_buttons;
} bt_keyboard_mouse_control_t;

typedef struct bt_keyboard bt_keyboard_t;

bt_keyboard_t *bt_keyboard_create(void);
void bt_keyboard_destroy(bt_keyboard_t *keyboard);
bool bt_keyboard_setup(bt_keyboard_t *keyboard, bt_keyboard_pid_handler_t handler);
bool bt_keyboard_devices_scan(bt_keyboard_t *keyboard, int seconds_wait_time);
bool bt_keyboard_devices_scan_ble_daemon(bt_keyboard_t *keyboard, int seconds_wait_time);
uint8_t bt_keyboard_get_battery_level(bt_keyboard_t *keyboard);
bool bt_keyboard_wait_for_low_event(bt_keyboard_t *keyboard, bt_keyboard_key_info_t *info, TickType_t duration);
bool bt_keyboard_wait_for_low_event_ccontrol(bt_keyboard_t *keyboard, bt_keyboard_key_info_ccontrol_t *info, TickType_t duration);
bool bt_keyboard_wait_for_low_event_mouse(bt_keyboard_t *keyboard, bt_keyboard_mouse_control_t *info, TickType_t duration);
char bt_keyboard_wait_for_ascii_char(bt_keyboard_t *keyboard, bool forever);
char bt_keyboard_get_ascii_char(bt_keyboard_t *keyboard);
void bt_keyboard_quick_reconnect(bt_keyboard_t *keyboard);
void bt_keyboard_set_leds(uint8_t leds);
bool bt_keyboard_is_connected(void);
void bt_keyboard_set_connected(bool value);
bool bt_keyboard_bt_device_found(void);
void bt_keyboard_set_bt_device_found(bool value);

#ifdef __cplusplus
}
#endif

#endif /* BT_KEYBOARD_H */
