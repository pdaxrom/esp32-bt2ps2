#include "status_led.hpp"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/task.h"

namespace {
void level(bool on) { gpio_set_level(GPIO_NUM_2, on); }
void flash(unsigned count, unsigned duration)
{
    for (unsigned i = 0; i < count; ++i) {
        level(true); vTaskDelay(pdMS_TO_TICKS(duration));
        level(false); vTaskDelay(pdMS_TO_TICKS(duration));
    }
}
}
bool StatusLed::begin()
{
    gpio_reset_pin(GPIO_NUM_2);
    gpio_set_direction(GPIO_NUM_2, GPIO_MODE_OUTPUT);
    codes_ = xQueueCreate(1, sizeof(uint32_t));
    return codes_ && xTaskCreate(task, "status_led", 2048, this, 2, nullptr) == pdPASS;
}
void StatusLed::show_passkey(uint32_t code)
{
    ESP_LOGW("Pairing", "Enter %06lu followed by ENTER", static_cast<unsigned long>(code));
    if (codes_) xQueueOverwrite(codes_, &code);
}
void StatusLed::activity() { activity_until_ = esp_timer_get_time() + 30000; }
void StatusLed::task(void *arg) { static_cast<StatusLed *>(arg)->run(); }
void StatusLed::run()
{
    for (;;) {
        uint32_t code;
        if (xQueueReceive(codes_, &code, pdMS_TO_TICKS(20)) == pdTRUE) {
            flash(10, 50);
            for (uint32_t divisor = 100000; divisor; divisor /= 10) {
                vTaskDelay(pdMS_TO_TICKS(2000));
                const unsigned digit = (code / divisor) % 10;
                if (digit) flash(digit, 200);
                else { level(true); vTaskDelay(pdMS_TO_TICKS(1500)); level(false); vTaskDelay(pdMS_TO_TICKS(200)); }
            }
            vTaskDelay(pdMS_TO_TICKS(1000)); flash(10, 50);
        }
        level(!pairing_.load() && esp_timer_get_time() >= activity_until_.load());
    }
}
