// Copyright (c) 2020 Guy Turcotte
//
// MIT License. Look at file licenses.txt for details.
//
// -----
//
// Original code from the bluetooth/esp_hid_host example of ESP-IDF license:
//
// Copyright 2017-2019 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

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

#include <vector>
#include <map>
#include <string>

#include "input_state.hpp"
#include <atomic>
#include <set>

// ESP-IDF Bluetooth host adapter. HID interpretation and input state live in pure modules.
class BluetoothHost {
public:
    using pid_handler = void(uint32_t);
    enum KeyLed : uint8_t {
        KEYBOARD_LED_NUMLOCK = 1, KEYBOARD_LED_CAPSLOCK = 2, KEYBOARD_LED_SCROLLLOCK = 4
    };
    bool setup(pid_handler *handler = nullptr);
    bool scan_pairing_devices(int seconds_wait_time = 5);
    bool scan_known_devices(int seconds_wait_time = 5);
    bool wait_for_input(input::Snapshot &snapshot, TickType_t wait) {
        return xQueueReceive(input_queue, &snapshot, wait) == pdTRUE;
    }
    static void set_leds(uint8_t leds);
    static std::atomic<bool> connected;
    static std::atomic<bool> pairing_pending;
    static std::atomic<uint32_t> connection_generation;
private:
    static constexpr const char *TAG = "BluetoothHost";
    static constexpr esp_bt_mode_t HIDH_IDLE_MODE = ESP_BT_MODE_IDLE;
    static constexpr esp_bt_mode_t HIDH_BTDM_MODE = ESP_BT_MODE_BTDM;
    static constexpr esp_bt_mode_t HID_HOST_MODE = HIDH_BTDM_MODE;
    static SemaphoreHandle_t bt_hidh_cb_semaphore, ble_hidh_cb_semaphore, led_device_map_semaphore;
    struct esp_hid_scan_result_t {
        struct esp_hid_scan_result_t *next;

        esp_bd_addr_t bda;
        const char *name;
        int8_t rssi;
        esp_hid_usage_t usage;
        esp_hid_transport_t transport; // BT, BLE or USB

        union {
            struct {
                esp_bt_cod_t cod;
                esp_bt_uuid_t uuid;
            } bt;
            struct {
                esp_ble_addr_type_t addr_type;
                uint16_t appearance;
            } ble;
        };
    };

    esp_hid_scan_result_t *bt_scan_results = nullptr;
    esp_hid_scan_result_t *ble_scan_results = nullptr;
    size_t num_bt_scan_results = 0;
    size_t num_ble_scan_results = 0;

    static void hidh_callback(void *handler_args, esp_event_base_t base, int32_t id, void *event_data);

    static void bt_gap_event_handler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    static void ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);

    static const char *ble_addr_type_str(esp_ble_addr_type_t ble_addr_type);
    static const char *ble_gap_evt_str(uint8_t event);
    static const char *bt_gap_evt_str(uint8_t event);
    static const char *ble_key_type_str(esp_ble_key_type_t key_type);

    static const char *gap_bt_prop_type_names[];
    static const char *ble_gap_evt_names[];
    static const char *bt_gap_evt_names[];
    static const char *ble_addr_type_names[];


    void handle_bt_device_result(esp_bt_gap_cb_param_t *param);
    void handle_ble_device_result(esp_ble_gap_cb_param_t *scan_rst);

    static void esp_hid_scan_results_free(esp_hid_scan_result_t *results);
    esp_hid_scan_result_t *find_scan_result(esp_bd_addr_t bda, esp_hid_scan_result_t *results);

    void add_bt_scan_result(esp_bd_addr_t bda,
                            esp_bt_cod_t *cod,
                            esp_bt_uuid_t *uuid,
                            uint8_t *name,
                            uint8_t name_len,
                            int rssi);

    void add_ble_scan_result(esp_bd_addr_t bda,
                             esp_ble_addr_type_t addr_type,
                             uint16_t appearance,
                             uint8_t *name,
                             uint8_t name_len,
                             int rssi);

    bool scan_and_connect(int seconds, bool pairing);
    static std::vector<esp_ble_bond_dev_t> bonded_devices();

    void print_uuid(esp_bt_uuid_t *uuid);

    esp_err_t start_ble_scan(uint32_t seconds);
    esp_err_t start_bt_scan(uint32_t seconds);
    esp_err_t esp_hid_scan(uint32_t seconds, size_t *num_results, esp_hid_scan_result_t **results, bool enable_bt_classic);

    static bool find_output_report(esp_hidh_dev_t *dev, size_t &map_index, size_t &report_id);

    inline void set_battery_level(uint8_t level)
    {
        battery_level = level;
    }


    struct LedReport {
        esp_hidh_dev_t *dev;
        size_t map_index, report_id;
    };
    static std::map<esp_hidh_dev_t *, LedReport> led_device_map;
    static std::map<input::Source, hid::Report> input_reports;
    static std::set<uintptr_t> connected_devices;
    static input::State input_state;
    static QueueHandle_t input_queue;
    static std::atomic<QueueHandle_t> led_queue;
    static std::atomic<uint8_t> desired_leds;
    static void notify_led_task();
    static void led_task(void *);
    static void publish_input();
    static void register_reports(esp_hidh_dev_t *dev);
    pid_handler *pairing_handler = nullptr;
    int8_t battery_level = -1;
};
