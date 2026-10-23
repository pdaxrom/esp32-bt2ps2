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

#include "bluetooth_host.hpp"

#include <cstring>
#include <algorithm>
#include <iterator>
#include <memory>

// ESP-IDF headers allow undefined CONFIG_* flags; our own conditions must be explicit.
#pragma GCC diagnostic error "-Wundef"

// uncomment to print all devices that were seen during a scan
#define GAP_DBG_PRINTF(...) printf(__VA_ARGS__)

#define SIZEOF_ARRAY(a) (sizeof(a) / sizeof(*a))

#define WAIT_BT_CB() xSemaphoreTake(bt_hidh_cb_semaphore, portMAX_DELAY)
#define SEND_BT_CB() xSemaphoreGive(bt_hidh_cb_semaphore)

#define WAIT_BLE_CB() xSemaphoreTake(ble_hidh_cb_semaphore, portMAX_DELAY)
#define SEND_BLE_CB() xSemaphoreGive(ble_hidh_cb_semaphore)

SemaphoreHandle_t BluetoothHost::bt_hidh_cb_semaphore = nullptr;
SemaphoreHandle_t BluetoothHost::ble_hidh_cb_semaphore = nullptr;
SemaphoreHandle_t BluetoothHost::led_device_map_semaphore = nullptr;

const char *BluetoothHost::gap_bt_prop_type_names[] = {"", "BDNAME", "COD", "RSSI", "EIR"};
const char *BluetoothHost::ble_gap_evt_names[] = {"ADV_DATA_SET_COMPLETE", "SCAN_RSP_DATA_SET_COMPLETE", "SCAN_PARAM_SET_COMPLETE", "SCAN_RESULT", "ADV_DATA_RAW_SET_COMPLETE", "SCAN_RSP_DATA_RAW_SET_COMPLETE", "ADV_START_COMPLETE", "SCAN_START_COMPLETE", "AUTH_CMPL", "KEY", "SEC_REQ", "PASSKEY_NOTIF", "PASSKEY_REQ", "OOB_REQ", "LOCAL_IR", "LOCAL_ER", "NC_REQ", "ADV_STOP_COMPLETE", "SCAN_STOP_COMPLETE", "SET_STATIC_RAND_ADDR", "UPDATE_CONN_PARAMS", "SET_PKT_LENGTH_COMPLETE", "SET_LOCAL_PRIVACY_COMPLETE", "REMOVE_BOND_DEV_COMPLETE", "CLEAR_BOND_DEV_COMPLETE", "GET_BOND_DEV_COMPLETE", "READ_RSSI_COMPLETE", "UPDATE_WHITELIST_COMPLETE"};
const char *BluetoothHost::bt_gap_evt_names[] = {"DISC_RES", "DISC_STATE_CHANGED", "RMT_SRVCS", "RMT_SRVC_REC", "AUTH_CMPL", "PIN_REQ", "CFM_REQ", "KEY_NOTIF", "KEY_REQ", "READ_RSSI_DELTA"};
const char *BluetoothHost::ble_addr_type_names[] = {"PUBLIC", "RANDOM", "RPA_PUBLIC", "RPA_RANDOM"};

static BluetoothHost *host_instance = nullptr;
std::atomic<bool> BluetoothHost::connected{false};
std::atomic<bool> BluetoothHost::pairing_pending{false};

const char *
BluetoothHost::ble_addr_type_str(esp_ble_addr_type_t ble_addr_type)
{
    if (ble_addr_type > BLE_ADDR_TYPE_RPA_RANDOM) {
        return "UNKNOWN";
    }
    return ble_addr_type_names[ble_addr_type];
}

const char *
BluetoothHost::ble_gap_evt_str(uint8_t event)
{
    if (event >= SIZEOF_ARRAY(ble_gap_evt_names)) {
        return "UNKNOWN";
    }
    return ble_gap_evt_names[event];
}

const char *
BluetoothHost::bt_gap_evt_str(uint8_t event)
{
    if (event >= SIZEOF_ARRAY(bt_gap_evt_names)) {
        return "UNKNOWN";
    }
    return bt_gap_evt_names[event];
}

const char *
BluetoothHost::ble_key_type_str(esp_ble_key_type_t key_type)
{
    const char *key_str = nullptr;
    switch (key_type) {
    case ESP_LE_KEY_NONE:
        key_str = "ESP_LE_KEY_NONE";
        break;
    case ESP_LE_KEY_PENC:
        key_str = "ESP_LE_KEY_PENC";
        break;
    case ESP_LE_KEY_PID:
        key_str = "ESP_LE_KEY_PID";
        break;
    case ESP_LE_KEY_PCSRK:
        key_str = "ESP_LE_KEY_PCSRK";
        break;
    case ESP_LE_KEY_PLK:
        key_str = "ESP_LE_KEY_PLK";
        break;
    case ESP_LE_KEY_LLK:
        key_str = "ESP_LE_KEY_LLK";
        break;
    case ESP_LE_KEY_LENC:
        key_str = "ESP_LE_KEY_LENC";
        break;
    case ESP_LE_KEY_LID:
        key_str = "ESP_LE_KEY_LID";
        break;
    case ESP_LE_KEY_LCSRK:
        key_str = "ESP_LE_KEY_LCSRK";
        break;
    default:
        key_str = "INVALID BLE KEY TYPE";
        break;
    }

    return key_str;
}

void BluetoothHost::print_uuid(esp_bt_uuid_t *uuid)
{
    if (uuid->len == ESP_UUID_LEN_16) {
        GAP_DBG_PRINTF("UUID16: 0x%04x", uuid->uuid.uuid16);
    } else if (uuid->len == ESP_UUID_LEN_32) {
        GAP_DBG_PRINTF("UUID32: 0x%08lx", uuid->uuid.uuid32);
    } else if (uuid->len == ESP_UUID_LEN_128) {
        GAP_DBG_PRINTF("UUID128: %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x",
                       uuid->uuid.uuid128[0],
                       uuid->uuid.uuid128[1], uuid->uuid.uuid128[2], uuid->uuid.uuid128[3],
                       uuid->uuid.uuid128[4], uuid->uuid.uuid128[5], uuid->uuid.uuid128[6],
                       uuid->uuid.uuid128[7], uuid->uuid.uuid128[8], uuid->uuid.uuid128[9],
                       uuid->uuid.uuid128[10], uuid->uuid.uuid128[11], uuid->uuid.uuid128[12],
                       uuid->uuid.uuid128[13], uuid->uuid.uuid128[14], uuid->uuid.uuid128[15]);
    }
}

void BluetoothHost::esp_hid_scan_results_free(esp_hid_scan_result_t *results)
{
    esp_hid_scan_result_t *r = nullptr;
    while (results) {
        r = results;
        results = results->next;
        if (r->name != nullptr) {
            free((char *)r->name);
        }
        free(r);
    }
}

BluetoothHost::esp_hid_scan_result_t *
BluetoothHost::find_scan_result(esp_bd_addr_t bda, esp_hid_scan_result_t *results)
{
    esp_hid_scan_result_t *r = results;
    while (r) {
        if (memcmp(bda, r->bda, sizeof(esp_bd_addr_t)) == 0) {
            return r;
        }
        r = r->next;
    }
    return nullptr;
}

static bool is_bonded_device(const esp_bd_addr_t bda, const esp_ble_bond_dev_t *dev_list, int numBonded)
{
    for (int j = 0; j < numBonded; ++j) {
        if (memcmp(bda, dev_list[j].bd_addr, sizeof(esp_bd_addr_t)) == 0) {
            return true;
        }
    }
    return false;
}

void BluetoothHost::add_bt_scan_result(esp_bd_addr_t bda,
                                    esp_bt_cod_t *cod,
                                    esp_bt_uuid_t *uuid,
                                    uint8_t *name,
                                    uint8_t name_len,
                                    int rssi)
{
    esp_hid_scan_result_t *r = find_scan_result(bda, bt_scan_results);
    if (r) {
        // Some info may come later
        if (r->name == nullptr && name && name_len) {
            char *name_s = (char *)malloc(name_len + 1);
            if (name_s == nullptr) {
                ESP_LOGE(TAG, "Malloc result name failed!");
                return;
            }
            memcpy(name_s, name, name_len);
            name_s[name_len] = 0;
            r->name = (const char *)name_s;
        }
        if (r->bt.uuid.len == 0 && uuid->len) {
            memcpy(&r->bt.uuid, uuid, sizeof(esp_bt_uuid_t));
        }
        if (rssi != 0) {
            r->rssi = rssi;
        }
        return;
    }

    r = (esp_hid_scan_result_t *)malloc(sizeof(esp_hid_scan_result_t));

    if (r == nullptr) {
        ESP_LOGE(TAG, "Malloc bt_hidh_scan_result_t failed!");
        return;
    }

    r->transport = ESP_HID_TRANSPORT_BT;

    memcpy(r->bda, bda, sizeof(esp_bd_addr_t));
    memcpy(&r->bt.cod, cod, sizeof(esp_bt_cod_t));
    memcpy(&r->bt.uuid, uuid, sizeof(esp_bt_uuid_t));

    uint32_t cod_value;
    memcpy(&cod_value, cod, sizeof(cod_value));
    r->usage = esp_hid_usage_from_cod(cod_value);
    r->rssi = rssi;
    r->name = nullptr;

    if (name_len && name) {
        char *name_s = (char *)malloc(name_len + 1);
        if (name_s == nullptr) {
            free(r);
            ESP_LOGE(TAG, "Malloc result name failed!");
            return;
        }
        memcpy(name_s, name, name_len);
        name_s[name_len] = 0;
        r->name = (const char *)name_s;
    }
    r->next = bt_scan_results;
    bt_scan_results = r;
    num_bt_scan_results++;
}

void BluetoothHost::add_ble_scan_result(esp_bd_addr_t bda,
                                     esp_ble_addr_type_t addr_type,
                                     uint16_t appearance,
                                     uint8_t *name,
                                     uint8_t name_len,
                                     int rssi)
{
    if (find_scan_result(bda, ble_scan_results)) {
        ESP_LOGW(TAG, "Result already exists!");
        return;
    }

    esp_hid_scan_result_t *r = (esp_hid_scan_result_t *)malloc(sizeof(esp_hid_scan_result_t));

    if (r == nullptr) {
        ESP_LOGE(TAG, "Malloc ble_hidh_scan_result_t failed!");
        return;
    }

    r->transport = ESP_HID_TRANSPORT_BLE;

    memcpy(r->bda, bda, sizeof(esp_bd_addr_t));

    r->ble.appearance = appearance;
    r->ble.addr_type = addr_type;
    r->usage = esp_hid_usage_from_appearance(appearance);
    r->rssi = rssi;
    r->name = nullptr;

    if (name_len && name) {
        char *name_s = (char *)malloc(name_len + 1);
        if (name_s == nullptr) {
            free(r);
            ESP_LOGE(TAG, "Malloc result name failed!");
            return;
        }
        memcpy(name_s, name, name_len);
        name_s[name_len] = 0;
        r->name = (const char *)name_s;
    }

    r->next = ble_scan_results;
    ble_scan_results = r;
    num_ble_scan_results++;
}

bool BluetoothHost::setup(pid_handler *handler)
{
    esp_err_t ret;
    const esp_bt_mode_t mode = HID_HOST_MODE;

    if (host_instance != nullptr) {
        ESP_LOGE(TAG, "Setup called more than once. Only one instance of BluetoothHost is allowed.");
        return false;
    }

    host_instance = this;

    pairing_handler = handler;
    input_queue = xQueueCreate(32, sizeof(input::Snapshot));
    led_queue = xQueueCreate(1, sizeof(uint8_t));
    if (!input_queue || !led_queue.load()) return false;

    if (HID_HOST_MODE == HIDH_IDLE_MODE) {
        ESP_LOGE(TAG, "Please turn on BT HID host or BLE!");
        return false;
    }

    bt_hidh_cb_semaphore = xSemaphoreCreateBinary();
    if (bt_hidh_cb_semaphore == nullptr) {
        ESP_LOGE(TAG, "xSemaphoreCreateMutex failed!");
        return false;
    }

    ble_hidh_cb_semaphore = xSemaphoreCreateBinary();
    if (ble_hidh_cb_semaphore == nullptr) {
        ESP_LOGE(TAG, "xSemaphoreCreateMutex failed!");
        vSemaphoreDelete(bt_hidh_cb_semaphore);
        bt_hidh_cb_semaphore = nullptr;
        return false;
    }

    led_device_map_semaphore = xSemaphoreCreateMutex();
    if (led_device_map_semaphore == nullptr) {
        ESP_LOGE(TAG, "xSemaphoreCreateMutex failed!");
        vSemaphoreDelete(bt_hidh_cb_semaphore);
        vSemaphoreDelete(ble_hidh_cb_semaphore);
        bt_hidh_cb_semaphore = nullptr;
        ble_hidh_cb_semaphore = nullptr;
        return false;
    }

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    bt_cfg.mode = mode;
    bt_cfg.bt_max_acl_conn = 3;
    bt_cfg.bt_max_sync_conn = 3;

    if ((ret = esp_bt_controller_init(&bt_cfg))) {
        ESP_LOGE(TAG, "esp_bt_controller_init failed: %d", ret);
        return false;
    }

    if ((ret = esp_bt_controller_enable(mode))) {
        ESP_LOGE(TAG, "esp_bt_controller_enable failed: %d", ret);
        return false;
    }

    if ((ret = esp_bluedroid_init())) {
        ESP_LOGE(TAG, "esp_bluedroid_init failed: %d", ret);
        return false;
    }

    if ((ret = esp_bluedroid_enable())) {
        ESP_LOGE(TAG, "esp_bluedroid_enable failed: %d", ret);
        return false;
    }

    // Classic Bluetooth GAP

    esp_bt_sp_param_t param_type = ESP_BT_SP_IOCAP_MODE;
    esp_bt_io_cap_t iocap = ESP_BT_IO_CAP_IO;
    esp_bt_gap_set_security_param(param_type, &iocap, sizeof(uint8_t));

    /*
    * Set default parameters for Legacy Pairing
    * Use fixed pin code
    */
    esp_bt_pin_type_t pin_type = ESP_BT_PIN_TYPE_FIXED;
    esp_bt_pin_code_t pin_code;
    pin_code[0] = '1';
    pin_code[1] = '2';
    pin_code[2] = '3';
    pin_code[3] = '4';
    esp_bt_gap_set_pin(pin_type, 4, pin_code);

    if ((ret = esp_bt_gap_register_callback(bt_gap_event_handler))) {
        ESP_LOGE(TAG, "esp_bt_gap_register_callback failed: %d", ret);
        return false;
    }

    // Allow BT devices to connect back to us
    if ((ret = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE))) {
        ESP_LOGE(TAG, "esp_bt_gap_set_scan_mode failed: %d", ret);
        return false;
    }

    // BLE GAP

    if ((ret = esp_ble_gap_register_callback(ble_gap_event_handler))) {
        ESP_LOGE(TAG, "esp_ble_gap_register_callback failed: %d", ret);
        return false;
    }

    ESP_ERROR_CHECK(esp_ble_gattc_register_callback(esp_hidh_gattc_event_handler));
    esp_hidh_config_t config = {
        .callback = hidh_callback,
        .event_stack_size = 4 * 1024, // Required with ESP-IDF 4.4
        .callback_arg = this       // idem
    };
    ESP_ERROR_CHECK(esp_hidh_init(&config));

    /* set the security iocap & auth_req & key size & init key response key parameters to the stack*/
    esp_ble_auth_req_t auth_req = ESP_LE_AUTH_REQ_SC_MITM_BOND; // bonding with peer device after authentication
    uint8_t key_size = 16;                                      // the key size should be 7~16 bytes
    uint8_t init_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t rsp_key = ESP_BLE_ENC_KEY_MASK | ESP_BLE_ID_KEY_MASK;
    uint8_t oob_support = ESP_BLE_OOB_DISABLE;
    if (esp_ble_gap_set_security_param(ESP_BLE_SM_AUTHEN_REQ_MODE, &auth_req, sizeof(uint8_t)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_set_security_param() failed on setting param: auth_req");
    }
    if (esp_ble_gap_set_security_param(ESP_BLE_SM_IOCAP_MODE, &iocap, sizeof(uint8_t)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_set_security_param() failed on setting param: iocap");
    }
    if (esp_ble_gap_set_security_param(ESP_BLE_SM_MAX_KEY_SIZE, &key_size, sizeof(uint8_t)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_set_security_param() failed on setting param: key_size");
    }
    if (esp_ble_gap_set_security_param(ESP_BLE_SM_OOB_SUPPORT, &oob_support, sizeof(uint8_t)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_set_security_param() failed on setting param: oob_support");
    }
    /* If your BLE device act as a Slave, the init_key means you hope which types of key of the master should distribute to you,
    and the response key means which key you can distribute to the Master;
    If your BLE device act as a master, the response key means you hope which types of key of the slave should distribute to you,
    and the init key means which key you can distribute to the slave. */
    if (esp_ble_gap_set_security_param(ESP_BLE_SM_SET_INIT_KEY, &init_key, sizeof(uint8_t)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_set_security_param() failed on setting param: init_key");
    }
    if (esp_ble_gap_set_security_param(ESP_BLE_SM_SET_RSP_KEY, &rsp_key, sizeof(uint8_t)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_set_security_param() failed on setting param: rsp_key");
    }

    if (xTaskCreate(led_task, "keyboard_leds", 3072, nullptr, 3, nullptr) != pdPASS) return false;

    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, ESP_PWR_LVL_P9);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, ESP_PWR_LVL_P9);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, ESP_PWR_LVL_P9);

    int pwrAdv = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV);
    int pwrScan = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_SCAN);
    int pwrDef = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);
    ESP_LOGI("Bluetooth setup:", "Power Settings: (ADV,SCAN,DEFAULT) %u, %u, %u", pwrAdv, pwrScan,
             pwrDef); // all should show index7, aka +9dbm
    return true;
}

void BluetoothHost::handle_bt_device_result(esp_bt_gap_cb_param_t *param)
{
    GAP_DBG_PRINTF("BT : " ESP_BD_ADDR_STR, ESP_BD_ADDR_HEX(param->disc_res.bda));

    uint32_t codv = 0;
    esp_bt_cod_t *cod = (esp_bt_cod_t *)&codv;
    int8_t rssi = 0;
    uint8_t *name = nullptr;
    uint8_t name_len = 0;
    esp_bt_uuid_t uuid;

    uuid.len = ESP_UUID_LEN_16;
    uuid.uuid.uuid16 = 0;

    for (int i = 0; i < param->disc_res.num_prop; i++) {
        esp_bt_gap_dev_prop_t *prop = &param->disc_res.prop[i];
        if (!prop->val || prop->len <= 0) continue;
        if (prop->type != ESP_BT_GAP_DEV_PROP_EIR) {
            const auto type = static_cast<unsigned>(prop->type);
            GAP_DBG_PRINTF(", %s: ", type < SIZEOF_ARRAY(gap_bt_prop_type_names) ? gap_bt_prop_type_names[type] : "UNKNOWN");
        }
        if (prop->type == ESP_BT_GAP_DEV_PROP_BDNAME) {
            name = (uint8_t *)prop->val;
            name_len = strnlen((const char *)name, std::min<int>(prop->len, UINT8_MAX));
            GAP_DBG_PRINTF("%.*s", int(name_len), (const char *)name);
        } else if (prop->type == ESP_BT_GAP_DEV_PROP_RSSI && prop->len >= 1) {
            rssi = *((int8_t *)prop->val);
            GAP_DBG_PRINTF("%d", rssi);
        } else if (prop->type == ESP_BT_GAP_DEV_PROP_COD && prop->len >= sizeof(codv)) {
            memcpy(&codv, prop->val, sizeof(uint32_t));
            GAP_DBG_PRINTF("major: %s, minor: %d, service: 0x%03x", esp_hid_cod_major_str(cod->major), cod->minor, cod->service);
        } else if (prop->type == ESP_BT_GAP_DEV_PROP_EIR) {
            uint8_t len = 0;
            uint8_t *data = 0;

            data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_CMPL_16BITS_UUID, &len);

            if (data == nullptr) {
                data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_INCMPL_16BITS_UUID, &len);
            }

            if (data && len == ESP_UUID_LEN_16) {
                uuid.len = ESP_UUID_LEN_16;
                uuid.uuid.uuid16 = data[0] + (data[1] << 8);
                GAP_DBG_PRINTF(", ");
                print_uuid(&uuid);
                continue;
            }

            data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_CMPL_32BITS_UUID, &len);

            if (data == nullptr) {
                data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_INCMPL_32BITS_UUID, &len);
            }

            if (data && len == ESP_UUID_LEN_32) {
                uuid.len = len;
                memcpy(&uuid.uuid.uuid32, data, sizeof(uint32_t));
                GAP_DBG_PRINTF(", ");
                print_uuid(&uuid);
                continue;
            }

            data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_CMPL_128BITS_UUID, &len);

            if (data == nullptr) {
                data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_INCMPL_128BITS_UUID, &len);
            }

            if (data && len == ESP_UUID_LEN_128) {
                uuid.len = len;
                memcpy(uuid.uuid.uuid128, (uint8_t *)data, len);
                GAP_DBG_PRINTF(", ");
                print_uuid(&uuid);
                continue;
            }

            // try to find a name
            if (name == nullptr) {
                data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &len);

                if (data == nullptr) {
                    data = esp_bt_gap_resolve_eir_data((uint8_t *)prop->val, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &len);
                }

                if (data && len) {
                    name = data;
                    name_len = len;
                    GAP_DBG_PRINTF(", NAME: ");
                    for (int x = 0; x < len; x++) {
                        GAP_DBG_PRINTF("%c", (char)data[x]);
                    }
                }
            }
        }
    }
    GAP_DBG_PRINTF("\n");

    if ((cod->major == ESP_BT_COD_MAJOR_DEV_PERIPHERAL) ||
            (find_scan_result(param->disc_res.bda, bt_scan_results) != nullptr)) {
        add_bt_scan_result(param->disc_res.bda, cod, &uuid, name, name_len, rssi);
    }
}

void BluetoothHost::bt_gap_event_handler(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    switch (event) {
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT: {
        ESP_LOGV(TAG, "BT GAP DISC_STATE %s", (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED) ? "START" : "STOP");
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED) {
            SEND_BT_CB();
        }
        break;
    }
    case ESP_BT_GAP_DISC_RES_EVT: {
        host_instance->handle_bt_device_result(param);
        break;
    }
    case ESP_BT_GAP_KEY_NOTIF_EVT:
        ESP_LOGV(TAG, "BT GAP KEY_NOTIF passkey:%d", param->key_notif.passkey);
        if (host_instance->pairing_handler != nullptr) {
            (*host_instance->pairing_handler)(param->key_notif.passkey);
        }
        break;
    case ESP_BT_GAP_MODE_CHG_EVT:
        ESP_LOGV(TAG, "BT GAP MODE_CHG_EVT mode:%d", param->mode_chg.mode);
        break;
    default:
        ESP_LOGV(TAG, "BT GAP EVENT %s", bt_gap_evt_str(event));
        break;
    }
}

void BluetoothHost::handle_ble_device_result(esp_ble_gap_cb_param_t *param)
{
    uint16_t uuid = 0;
    uint16_t appearance = 0;
    char name[64] = "";

    uint8_t uuid_len = 0;
    uint8_t *uuid_d = esp_ble_resolve_adv_data_by_type(param->scan_rst.ble_adv,
                      param->scan_rst.adv_data_len + param->scan_rst.scan_rsp_len, ESP_BLE_AD_TYPE_16SRV_CMPL, &uuid_len);

    if (uuid_d != nullptr && uuid_len >= 2) {
        uuid = uuid_d[0] + (uuid_d[1] << 8);
    }

    uint8_t appearance_len = 0;
    uint8_t *appearance_d = esp_ble_resolve_adv_data_by_type(param->scan_rst.ble_adv,
                            param->scan_rst.adv_data_len + param->scan_rst.scan_rsp_len, ESP_BLE_AD_TYPE_APPEARANCE, &appearance_len);

    if (appearance_d != nullptr && appearance_len >= 2) {
        appearance = appearance_d[0] + (appearance_d[1] << 8);
    }

    uint8_t adv_name_len = 0;
    uint8_t *adv_name = esp_ble_resolve_adv_data_by_type(param->scan_rst.ble_adv,
                        param->scan_rst.adv_data_len + param->scan_rst.scan_rsp_len, ESP_BLE_AD_TYPE_NAME_CMPL, &adv_name_len);

    if (adv_name == nullptr) {
        adv_name = esp_ble_resolve_adv_data_by_type(param->scan_rst.ble_adv,
                   param->scan_rst.adv_data_len + param->scan_rst.scan_rsp_len,ESP_BLE_AD_TYPE_NAME_SHORT, &adv_name_len);
    }

    if (adv_name != nullptr && adv_name_len) {
        const size_t copied = std::min<size_t>(adv_name_len, sizeof(name) - 1);
        memcpy(name, adv_name, copied);
        name[copied] = 0;
    }

    GAP_DBG_PRINTF("BLE: " ESP_BD_ADDR_STR ", ", ESP_BD_ADDR_HEX(param->scan_rst.bda));
    GAP_DBG_PRINTF("RSSI: %d, ", param->scan_rst.rssi);
    GAP_DBG_PRINTF("UUID: 0x%04x, ", uuid);
    GAP_DBG_PRINTF("APPEARANCE: 0x%04x, ", appearance);
    GAP_DBG_PRINTF("ADDR_TYPE: '%s'", ble_addr_type_str(param->scan_rst.ble_addr_type));

    if (adv_name_len) {
        GAP_DBG_PRINTF(", NAME: '%s'", name);
    }
    GAP_DBG_PRINTF("\n");

    const auto bonded = bonded_devices();

    const bool isLastBonded = is_bonded_device(param->scan_rst.bda, bonded.data(), bonded.size());

    if (uuid == ESP_GATT_UUID_HID_SVC || appearance == ESP_HID_APPEARANCE_MOUSE
            || appearance == ESP_HID_APPEARANCE_KEYBOARD || isLastBonded == true) {
        add_ble_scan_result(param->scan_rst.bda,
                            param->scan_rst.ble_addr_type,
                            appearance, adv_name, adv_name_len,
                            param->scan_rst.rssi);
    }

}

void BluetoothHost::ble_gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event) {

    // SCAN

    case ESP_GAP_BLE_SCAN_PARAM_SET_COMPLETE_EVT: {
        ESP_LOGV(TAG, "BLE GAP EVENT SCAN_PARAM_SET_COMPLETE");
        SEND_BLE_CB();
        break;
    }
    case ESP_GAP_BLE_SCAN_RESULT_EVT: {
        switch (param->scan_rst.search_evt) {
        case ESP_GAP_SEARCH_INQ_RES_EVT: {
            host_instance->handle_ble_device_result(param);
            break;
        }
        case ESP_GAP_SEARCH_INQ_CMPL_EVT:
            ESP_LOGV(TAG, "BLE GAP EVENT SCAN DONE: %d", param->scan_rst.num_resps);
            SEND_BLE_CB();
            break;
        default:
            break;
        }
        break;
    }
    case ESP_GAP_BLE_SCAN_STOP_COMPLETE_EVT: {
        ESP_LOGV(TAG, "BLE GAP EVENT SCAN CANCELED");
        break;
    }

    // ADVERTISEMENT

    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        ESP_LOGV(TAG, "BLE GAP ADV_DATA_SET_COMPLETE");
        break;

    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        ESP_LOGV(TAG, "BLE GAP ADV_START_COMPLETE");
        break;

    // AUTHENTICATION

    case ESP_GAP_BLE_AUTH_CMPL_EVT:
        if (!param->ble_security.auth_cmpl.success) {
            ESP_LOGE(TAG, "BLE GAP AUTH ERROR: 0x%x", param->ble_security.auth_cmpl.fail_reason);
        } else {
            ESP_LOGV(TAG, "BLE GAP AUTH SUCCESS");
        }
        break;

    case ESP_GAP_BLE_KEY_EVT: // shows the ble key info share with peer device to the user.
        ESP_LOGV(TAG, "BLE GAP KEY type = %s", ble_key_type_str(param->ble_security.ble_key.key_type));
        break;

    case ESP_GAP_BLE_PASSKEY_NOTIF_EVT: // ESP_IO_CAP_OUT
        // The app will receive this evt when the IO has Output capability and the peer device IO has Input capability.
        // Show the passkey number to the user to input it in the peer device.
        ESP_LOGV(TAG, "BLE GAP PASSKEY_NOTIF passkey:%d", param->ble_security.key_notif.passkey);
        if (host_instance->pairing_handler != nullptr) {
            (*host_instance->pairing_handler)(param->ble_security.key_notif.passkey);
        }
        break;

    case ESP_GAP_BLE_NC_REQ_EVT: // ESP_IO_CAP_IO
        // The app will receive this event when the IO has DisplayYesNO capability and the peer device IO also has DisplayYesNo capability.
        // show the passkey number to the user to confirm it with the number displayed by peer device.
        ESP_LOGV(TAG, "BLE GAP NC_REQ passkey:%d", param->ble_security.key_notif.passkey);
        esp_ble_confirm_reply(param->ble_security.key_notif.bd_addr, true);
        break;

    case ESP_GAP_BLE_PASSKEY_REQ_EVT: // ESP_IO_CAP_IN
        // The app will receive this evt when the IO has Input capability and the peer device IO has Output capability.
        // See the passkey number on the peer device and send it back.
        ESP_LOGV(TAG, "BLE GAP PASSKEY_REQ");
        // esp_ble_passkey_reply(param->ble_security.ble_req.bd_addr, true, 1234);
        break;

    case ESP_GAP_BLE_SEC_REQ_EVT:
        ESP_LOGV(TAG, "BLE GAP SEC_REQ");
        // Send the positive(true) security response to the peer device to accept the security request.
        // If not accept the security request, should send the security response with negative(false) accept value.
        esp_ble_gap_security_rsp(param->ble_security.ble_req.bd_addr, true);
        break;

    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:

        ESP_LOGI(TAG, "BLE GAP UPDATE_CONN_PARAMS min_int: %u max_int: %u latency: %u conn_int: %u timeout: %u",
                 param->update_conn_params.min_int,
                 param->update_conn_params.max_int,
                 param->update_conn_params.latency,
                 param->update_conn_params.conn_int,
                 param->update_conn_params.timeout);
        break;

    default:
        ESP_LOGV(TAG, "BLE GAP EVENT %s", ble_gap_evt_str(event));
        break;
    }
}

esp_err_t
BluetoothHost::start_bt_scan(uint32_t seconds)
{
    esp_err_t ret = ESP_OK;
    if ((ret = esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, (int)(seconds / 1.28), 0)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_bt_gap_start_discovery failed: %d", ret);
        return ret;
    }
    return ret;
}

esp_err_t
BluetoothHost::start_ble_scan(uint32_t seconds)
{
    static esp_ble_scan_params_t hid_scan_params = {
        .scan_type = BLE_SCAN_TYPE_ACTIVE,
        .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
        .scan_filter_policy = BLE_SCAN_FILTER_ALLOW_ALL,
        .scan_interval = 0x50,
        .scan_window = 0x30,
        .scan_duplicate = BLE_SCAN_DUPLICATE_ENABLE,
    };

    esp_err_t ret = ESP_OK;
    if ((ret = esp_ble_gap_set_scan_params(&hid_scan_params)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_set_scan_params failed: %d", ret);
        return ret;
    }
    WAIT_BLE_CB();

    if ((ret = esp_ble_gap_start_scanning(seconds)) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ble_gap_start_scanning failed: %d", ret);
        return ret;
    }
    return ret;
}

esp_err_t
BluetoothHost::esp_hid_scan(uint32_t seconds, size_t *num_results, esp_hid_scan_result_t **results,
                         bool enable_bt_classic = true)
{
    if (num_bt_scan_results || bt_scan_results || num_ble_scan_results || ble_scan_results) {
        ESP_LOGE(TAG, "There are old scan results. Free them first!");
        return ESP_FAIL;
    }

    if (start_ble_scan(seconds) == ESP_OK) {
        WAIT_BLE_CB();
    } else {
        return ESP_FAIL;
    }

    if (enable_bt_classic) {
        if (start_bt_scan(seconds) == ESP_OK) {
            WAIT_BT_CB();
        } else {
            esp_hid_scan_results_free(bt_scan_results);
            esp_hid_scan_results_free(ble_scan_results);
            bt_scan_results = ble_scan_results = nullptr;
            num_bt_scan_results = num_ble_scan_results = 0;
            return ESP_FAIL;
        }
    }

    *num_results = num_bt_scan_results + num_ble_scan_results;
    *results = bt_scan_results;

    if (num_bt_scan_results) {
        while (bt_scan_results->next != NULL) {
            bt_scan_results = bt_scan_results->next;
        }
        bt_scan_results->next = ble_scan_results;
    } else {
        *results = ble_scan_results;
    }

    num_bt_scan_results = 0;
    bt_scan_results = NULL;
    num_ble_scan_results = 0;
    ble_scan_results = NULL;

    return ESP_OK;
}

std::vector<esp_ble_bond_dev_t> BluetoothHost::bonded_devices()
{
    int count = esp_ble_get_bond_device_num();
    if (count <= 0) return {};
    std::vector<esp_ble_bond_dev_t> devices(count);
    if (esp_ble_get_bond_device_list(&count, devices.data()) != ESP_OK) {
        ESP_LOGW(TAG, "Cannot read bonded device list");
        return {};
    }
    devices.resize(count);
    return devices;
}

bool BluetoothHost::scan_and_connect(int seconds, bool pairing)
{
    const auto bonded = bonded_devices();
    size_t count = 0;
    esp_hid_scan_result_t *raw = nullptr;
    const esp_err_t status = esp_hid_scan(seconds, &count, &raw, pairing);
    std::unique_ptr<esp_hid_scan_result_t, decltype(&esp_hid_scan_results_free)> results(raw, esp_hid_scan_results_free);
    if (status != ESP_OK) return false;
    bool requested = false;
    const auto open = [](esp_hid_scan_result_t &device) {
        const auto address_type = device.transport == ESP_HID_TRANSPORT_BLE ? device.ble.addr_type : BLE_ADDR_TYPE_PUBLIC;
        return esp_hidh_dev_open(device.bda, device.transport, address_type) != nullptr;
    };
    for (auto *device = raw; device; device = device->next) {
        if (!is_bonded_device(device->bda, bonded.data(), bonded.size())) continue;
        requested |= open(*device);
    }
    if (requested || !pairing) return requested;
    // Prefer the last scan candidate, but an already-connected device returns nullptr.
    // Continue to the other candidates so it cannot prevent pairing another device.
    std::vector<esp_hid_scan_result_t *> candidates;
    for (auto *device = raw; device; device = device->next) candidates.push_back(device);
    for (auto candidate = candidates.rbegin(); candidate != candidates.rend(); ++candidate) {
        if (!open(**candidate)) continue;
        if ((*candidate)->transport == ESP_HID_TRANSPORT_BT) pairing_pending = true;
        return true; // OPEN, rather than an open request, confirms success.
    }
    return false;
}

bool BluetoothHost::scan_pairing_devices(int seconds) { return scan_and_connect(seconds, true); }
bool BluetoothHost::scan_known_devices(int seconds) { return scan_and_connect(seconds, false); }
