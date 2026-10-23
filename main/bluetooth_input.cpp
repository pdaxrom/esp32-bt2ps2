#include "bluetooth_host.hpp"
#include <algorithm>
#include <cstring>
#include <cstdio>
#include <type_traits>

static_assert(std::is_trivially_copyable<input::Snapshot>::value, "FreeRTOS queues copy snapshots as bytes");

std::map<input::Source, hid::Report> BluetoothHost::input_reports;
std::set<uintptr_t> BluetoothHost::connected_devices;
input::State BluetoothHost::input_state;
std::atomic<uint32_t> BluetoothHost::connection_generation{0};
QueueHandle_t BluetoothHost::input_queue = nullptr;
std::atomic<QueueHandle_t> BluetoothHost::led_queue{nullptr};
std::atomic<uint8_t> BluetoothHost::desired_leds{0};
std::map<esp_hidh_dev_t *, BluetoothHost::LedReport> BluetoothHost::led_device_map;

bool BluetoothHost::find_output_report(esp_hidh_dev_t *dev, size_t &map_index, size_t &report_id)
{
    size_t num_reports;
    esp_hid_report_item_t *reports = nullptr;

    esp_err_t ret = esp_hidh_dev_reports_get(dev, &num_reports, &reports);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error get reports: %s", esp_err_to_name(ret));
        return false;
    }

    for (int i = 0; i < num_reports; i++) {
        if (reports[i].report_type == ESP_HID_REPORT_TYPE_OUTPUT && reports[i].usage == ESP_HID_USAGE_KEYBOARD) {
            report_id = reports[i].report_id;
            map_index = reports[i].map_index;
            free(reports);
            return true;
        }
    }

    if (reports != nullptr) {
        free(reports);
    }

    return false;  // no OUTPUT report found
}


void BluetoothHost::register_reports(esp_hidh_dev_t *dev)
{
    size_t count = 0;
    esp_hid_raw_report_map_t *maps = nullptr;
    if (esp_hidh_dev_report_maps_get(dev, &count, &maps) != ESP_OK || !maps) {
        ESP_LOGE(TAG, "Cannot read HID descriptor");
        return;
    }
    for (size_t map = 0; map < count && map <= UINT8_MAX; ++map) {
        hid::Reports reports;
        if (!hid::parse(maps[map].data, maps[map].len, reports)) {
            ESP_LOGW(TAG, "Unsupported or malformed HID map %u", unsigned(map));
            continue;
        }
        for (auto &report : reports)
            input_reports[{reinterpret_cast<uintptr_t>(dev), uint8_t(map), report.first}] = std::move(report.second);
    }
}

void BluetoothHost::publish_input()
{
    const auto snapshot = input_state.snapshot();
    if (xQueueSend(input_queue, &snapshot, 0) == pdTRUE) return;
    // Preserve the latest held state and cumulative movement. Transient taps may be
    // lost under sustained overload, but releases and relative movement can recover.
    xQueueReset(input_queue);
    if (xQueueSend(input_queue, &snapshot, 0) != pdTRUE)
        ESP_LOGE(TAG, "Input resynchronization failed");
    ESP_LOGW(TAG, "Input queue overloaded; resynchronized current state");
}

void BluetoothHost::hidh_callback(void *arg, esp_event_base_t, int32_t id, void *event_data)
{
    auto *host = static_cast<BluetoothHost *>(arg);
    auto *event = static_cast<esp_hidh_event_data_t *>(event_data);
    switch (static_cast<esp_hidh_event_t>(id)) {
    case ESP_HIDH_OPEN_EVENT: {
        if (event->open.status != ESP_OK) {
            ESP_LOGW(TAG, "HID open failed: %d", event->open.status);
            break;
        }
        auto *dev = event->open.dev;
        connected_devices.insert(reinterpret_cast<uintptr_t>(dev));
        connected = !connected_devices.empty();
        ++connection_generation;
        register_reports(dev);
        if (esp_hidh_dev_transport_get(dev) == ESP_HID_TRANSPORT_BLE) {
            esp_gap_conn_params_t current{};
            if (esp_ble_get_current_conn_params(const_cast<uint8_t *>(esp_hidh_dev_bda_get(dev)), &current) == ESP_OK && current.timeout < 120) {
                esp_ble_conn_update_params_t params{};
                memcpy(params.bda, esp_hidh_dev_bda_get(dev), sizeof(params.bda));
                params.latency = current.latency; params.min_int = 6;
                params.max_int = current.interval; params.timeout = 400;
                esp_ble_gap_update_conn_params(&params);
            }
        }
        xSemaphoreTake(led_device_map_semaphore, portMAX_DELAY);
        size_t map, report;
        if (find_output_report(dev, map, report))
            led_device_map[dev] = {dev, map, report};
        xSemaphoreGive(led_device_map_semaphore);
        notify_led_task();
        ESP_LOGI(TAG, "HID connected: %s", esp_hidh_dev_name_get(dev));
        break;
    }
    case ESP_HIDH_INPUT_EVENT: {
        if (event->input.report_id > UINT8_MAX) break;
        input::Source source{reinterpret_cast<uintptr_t>(event->input.dev), event->input.map_index, static_cast<uint8_t>(event->input.report_id)};
        auto found = input_reports.find(source);
        if (found == input_reports.end()) {
            ESP_LOGW(TAG, "Unknown input map %u report %u", source.map, source.report);
            break;
        }
        hid::Input decoded;
        if (!hid::decode(found->second, event->input.data, event->input.length, decoded)) {
            ESP_LOGW(TAG, "Invalid or unsupported HID input");
            break;
        }
        if (input_state.update(source, decoded)) publish_input();
        else ESP_LOGW(TAG, "Too many simultaneous consumer usages");
        break;
    }
    case ESP_HIDH_CLOSE_EVENT: {
        auto *dev = event->close.dev;
        if (!dev) break;
        const auto device = reinterpret_cast<uintptr_t>(dev);
        input_state.disconnect(device);
        publish_input();
        connected_devices.erase(device);
        connected = !connected_devices.empty();
        for (auto it = input_reports.begin(); it != input_reports.end();) {
            if (it->first.device == device) it = input_reports.erase(it);
            else ++it;
        }
        xSemaphoreTake(led_device_map_semaphore, portMAX_DELAY);
        led_device_map.erase(dev);
        xSemaphoreGive(led_device_map_semaphore);
        ESP_LOGI(TAG, "HID disconnected");
        break;
    }
    case ESP_HIDH_BATTERY_EVENT: host->set_battery_level(event->battery.level); break;
    default: break;
    }
}

void BluetoothHost::set_leds(uint8_t leds)
{
    desired_leds.store(leds);
    notify_led_task();
}
void BluetoothHost::notify_led_task()
{
    const uint8_t notification = 0;
    if (auto queue = led_queue.load()) xQueueOverwrite(queue, &notification);
}
void BluetoothHost::led_task(void *)
{
    uint8_t leds;
    for (;;) {
        if (xQueueReceive(led_queue.load(), &leds, portMAX_DELAY) != pdTRUE) continue;
        leds = desired_leds.load();
        xSemaphoreTake(led_device_map_semaphore, portMAX_DELAY);
        for (auto &pair : led_device_map) {
            auto &report = pair.second;
            auto result = esp_hidh_dev_output_set(report.dev, report.map_index, report.report_id, &leds, 1);
            if (result != ESP_OK) ESP_LOGW(TAG, "Keyboard LED output: %s", esp_err_to_name(result));
        }
        xSemaphoreGive(led_device_map_semaphore);
    }
}
