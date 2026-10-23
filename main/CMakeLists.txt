set(gpio_component "")
if(EXISTS "$ENV{IDF_PATH}/components/esp_driver_gpio")
    set(gpio_component esp_driver_gpio)
elseif(EXISTS "$ENV{IDF_PATH}/components/driver")
    set(gpio_component driver)
else()
    message(FATAL_ERROR "No GPIO driver component found under IDF_PATH. Please install esp_driver_gpio (IDF >=5.1) or driver (older IDF).")
endif()

idf_component_register(SRCS "main.cpp" "bluetooth_host.cpp" "bluetooth_input.cpp"
    "hid_report.cpp" "input_state.cpp" "input_bridge.cpp"
    "status_led.cpp" "connection_manager.cpp" "ps2_bus.cpp" "ps2_keyboard.cpp" "ps2_mouse.cpp"
                       INCLUDE_DIRS "../include"
                       REQUIRES esp_hid nvs_flash ${gpio_component})
