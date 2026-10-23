// ESP32-BT2PS2. Copyright Humberto Mockel - Hamcode - 2024.
#include "bluetooth_host.hpp"
#include "connection_manager.hpp"
#include "input_bridge.hpp"
#include "ps2_output.hpp"
#include "status_led.hpp"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "nvs_flash.h"

namespace {
constexpr int KB_CLK_PIN = 22, KB_DATA_PIN = 23;
constexpr int MOUSE_CLK_PIN = 19, MOUSE_DATA_PIN = 21;
constexpr bool PAIR_ON_START = true;
esp32_ps2dev::PS2Keyboard keyboard(KB_CLK_PIN, KB_DATA_PIN);
esp32_ps2dev::PS2Mouse mouse(MOUSE_CLK_PIN, MOUSE_DATA_PIN);
BluetoothHost bluetooth;
StatusLed status;
ConnectionManager connections(bluetooth, status);

void pairing_code(uint32_t code) { status.show_passkey(code); }
void keyboard_leds(uint8_t leds)
{
    uint8_t output = 0;
    if (leds & esp32_ps2dev::PS2Keyboard::KEYBOARD_LED_SCROLLLOCK) output |= BluetoothHost::KEYBOARD_LED_SCROLLLOCK;
    if (leds & esp32_ps2dev::PS2Keyboard::KEYBOARD_LED_NUMLOCK) output |= BluetoothHost::KEYBOARD_LED_NUMLOCK;
    if (leds & esp32_ps2dev::PS2Keyboard::KEYBOARD_LED_CAPSLOCK) output |= BluetoothHost::KEYBOARD_LED_CAPSLOCK;
    BluetoothHost::set_leds(output);
}
void configure_pins()
{
    gpio_config_t config{};
    config.mode = GPIO_MODE_OUTPUT_OD;
    config.pin_bit_mask = (1ULL << KB_CLK_PIN) | (1ULL << KB_DATA_PIN) | (1ULL << MOUSE_CLK_PIN) | (1ULL << MOUSE_DATA_PIN);
    ESP_ERROR_CHECK(gpio_config(&config));
    config.mode = GPIO_MODE_INPUT;
    config.pin_bit_mask = 1ULL << GPIO_NUM_0;
    config.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&config));
}
}
extern "C" void app_main()
{
    configure_pins();
    ESP_ERROR_CHECK(status.begin() ? ESP_OK : ESP_ERR_NO_MEM);
    // Bring up PS/2 before Bluetooth so BIOS commands can be answered during boot.
    keyboard.set_leds_callback(keyboard_leds);
    mouse.begin(); keyboard.begin();
    esp_err_t result = nvs_flash_init();
    if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase()); result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(result);
    ESP_ERROR_CHECK(bluetooth.setup(pairing_code) ? ESP_OK : ESP_FAIL);
    ESP_ERROR_CHECK(connections.begin(PAIR_ON_START) ? ESP_OK : ESP_ERR_NO_MEM);
    Ps2Output output(keyboard, mouse);
    InputBridge bridge(output);
    for (;;) {
        input::Snapshot snapshot;
        if (bluetooth.wait_for_input(snapshot, pdMS_TO_TICKS(5))) {
            bridge.accept(snapshot); status.activity();
        }
        const int64_t now = esp_timer_get_time() / 1000;
        bridge.tick(now);
        connections.poll_button(now);
    }
}
