set(gpio_component "")
if(EXISTS "$ENV{IDF_PATH}/components/esp_driver_gpio")
    set(gpio_component esp_driver_gpio)
elseif(EXISTS "$ENV{IDF_PATH}/components/driver")
    set(gpio_component driver)
else()
    message(FATAL_ERROR "No GPIO driver component found under IDF_PATH. Please install esp_driver_gpio (IDF >=5.1) or driver (older IDF).")
endif()

idf_component_register(SRCS "main.cpp" "bt_keyboard.cpp" "esp32-ps2dev.cpp" "serial_mouse.cpp" "${app_sources}"
                       INCLUDE_DIRS "../include"
                       REQUIRES esp_hid nvs_flash ${gpio_component})
