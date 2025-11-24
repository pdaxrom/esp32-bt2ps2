#include "esp32-ps2dev.h"
#include <array>
#include <algorithm>

#define NOP() asm volatile("nop")
#define HIGH 0x1
#define LOW 0x0

// Unomment following line to enable debug messages on the PS2DEV module
//#define _ESP32_PS2DEV_DEBUG_
#define TAG __FILE__

namespace esp32_ps2dev
{

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
      for (auto &entry : map)
      {
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
      assign_key(map, 0x64, scancodes::Key::K_BACKQUOTE);
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

    struct ConsumerMapping
    {
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
      for (const auto &entry : CONSUMER_MAPPINGS)
      {
        if (entry.usage == usage)
        {
          out_key = entry.key;
          return true;
        }
      }
      return false;
    }

    bool key_from_set3_code(uint8_t code, scancodes::Key &out_key)
    {
      for (size_t idx = 0; idx < scancodes::KEY_COUNT; ++idx)
      {
        if (scancodes::MAKE_SET3_CODES_LEN[idx] == 1 && scancodes::MAKE_SET3_CODES[idx][0] == code)
        {
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

    constexpr uint16_t RATE_CYCLE_TABLE[32] = {
        33, 37, 40, 46, 50, 54, 58, 63,
        67, 72, 77, 83, 90, 96, 105, 112,
        125, 133, 143, 154, 167, 182, 200, 222,
        250, 286, 313, 333, 370, 400, 435, 476};
    constexpr uint16_t DELAY_TABLE[4] = {250, 500, 750, 1000};
  } // namespace

  // THIS SECTION DEFINES THE FUNCTIONS USED BELOW AS IMPLEMENTED IN THE ARDUINO CORE FOR ESP32
  // THE CODE ON THIS FILE HAS BEEN PORTED FROM AN ARDUINO-IDE PROJECT SO THIS IS NECESSARY
  // I KNOW IT'S NOT IDEAL BUT I DO NOT HAVE TIME TO MODIFY ALL NOR THE PATIENCE
  // ALSO THIS IS VERY TIME SENSITIVE CODE SO BETTER LEFT ALONE

  BaseType_t xTaskCreateUniversal(TaskFunction_t pxTaskCode,
                                  const char *const pcName,
                                  const uint32_t usStackDepth,
                                  void *const pvParameters,
                                  UBaseType_t uxPriority,
                                  TaskHandle_t *const pxCreatedTask,
                                  const BaseType_t xCoreID)
  {
#ifndef CONFIG_FREERTOS_UNICORE
    if (xCoreID >= 0 && xCoreID < 2)
    {
      return xTaskCreatePinnedToCore(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask, xCoreID);
    }
    else
    {
#endif
      return xTaskCreate(pxTaskCode, pcName, usStackDepth, pvParameters, uxPriority, pxCreatedTask);
#ifndef CONFIG_FREERTOS_UNICORE
    }
#endif
  }

  extern void digitalWrite(uint8_t pin, uint8_t val)
  {
    gpio_set_level((gpio_num_t)pin, val);
  }

  extern int digitalRead(uint8_t pin)
  {
    return gpio_get_level((gpio_num_t)pin);
  }
  void delay(uint32_t ms)
  {
    vTaskDelay(ms / portTICK_PERIOD_MS);
  }

  unsigned long IRAM_ATTR millis()
  {
    return (unsigned long)(esp_timer_get_time() / 1000ULL);
  }

  unsigned long IRAM_ATTR micros()
  {
    return (unsigned long)(esp_timer_get_time());
  }

  void IRAM_ATTR delayMicroseconds(uint32_t us)
  {
    uint32_t m = micros();
    if (us)
    {
      uint32_t e = (m + us);
      if (m > e)
      { // overflow
        while (micros() > e)
        {
          NOP();
        }
      }
      while (micros() < e)
      {
        NOP();
      }
    }
  }

  // END OF ARDUINO CORE ADAPTATION

  PS2dev::PS2dev(int clk, int data)
  {
    _ps2clk = clk;
    _ps2data = data;
  }

  void PS2dev::config(UBaseType_t task_priority, BaseType_t task_core)
  {
    if (task_priority < 1)
    {
      task_priority = 1;
    }
    else if (task_priority > configMAX_PRIORITIES)
    {
      task_priority = configMAX_PRIORITIES - 1;
    }
    _config_task_priority = task_priority;
    _config_task_core = task_core;
  }

  void PS2dev::begin(BaseType_t core = DEFAULT_TASK_CORE)
  {
    gohi(_ps2clk);
    gohi(_ps2data);
    _mutex_bus = xSemaphoreCreateMutex();
    _queue_packet = xQueueCreate(PACKET_QUEUE_LENGTH, sizeof(PS2Packet));
    xTaskCreateUniversal(_taskfn_process_host_request, "process_host_request", 4096, this, _config_task_priority, &_task_process_host_request, core);
    xTaskCreateUniversal(_taskfn_send_packet, "send_packet", 4096, this, _config_task_priority - 1, &_task_send_packet, core);
  }

  void PS2dev::gohi(int pin)
  {
    digitalWrite(pin, HIGH);
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_INPUT);
  }
  void PS2dev::golo(int pin)
  {
    gpio_set_direction((gpio_num_t)pin, GPIO_MODE_OUTPUT_OD);
    digitalWrite(pin, LOW);
  }
  void PS2dev::ack()
  {
    delayMicroseconds(BYTE_INTERVAL_MICROS);
    write(0xFA);
    delayMicroseconds(BYTE_INTERVAL_MICROS);
  }
  int PS2dev::write(unsigned char data)
  {
    unsigned char i;
    unsigned char parity = 1;

    if (get_bus_state() != BusState::IDLE)
    {
      return -1;
    }

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    taskENTER_CRITICAL(&mux);

    golo(_ps2data);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    // device sends on falling clock
    golo(_ps2clk); // start bit
    delayMicroseconds(CLK_HALF_PERIOD_MICROS);
    gohi(_ps2clk);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

    for (i = 0; i < 8; i++)
    {
      if (data & 0x01)
      {
        gohi(_ps2data);
      }
      else
      {
        golo(_ps2data);
      }
      delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
      golo(_ps2clk);
      delayMicroseconds(CLK_HALF_PERIOD_MICROS);
      gohi(_ps2clk);
      delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

      parity = parity ^ (data & 0x01);
      data = data >> 1;
    }
    // parity bit
    if (parity)
    {
      gohi(_ps2data);
    }
    else
    {
      golo(_ps2data);
    }
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    golo(_ps2clk);
    delayMicroseconds(CLK_HALF_PERIOD_MICROS);
    gohi(_ps2clk);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

    // stop bit
    gohi(_ps2data);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    golo(_ps2clk);
    delayMicroseconds(CLK_HALF_PERIOD_MICROS);
    gohi(_ps2clk);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

    taskEXIT_CRITICAL(&mux);

    return 0;
  }
  int PS2dev::write_wait_idle(uint8_t data, uint64_t timeout_micros)
  {
    uint64_t start_time = micros();
    while (get_bus_state() != BusState::IDLE)
    {
      if (micros() - start_time > timeout_micros)
      {
        return -1;
      }
    }
    return write(data);
  }
  int PS2dev::read(unsigned char *value, uint64_t timeout_ms)
  {
    unsigned int data = 0x00;
    unsigned int bit = 0x01;

    unsigned char calculated_parity = 1;
    unsigned char received_parity = 0;

    // wait for data line to go low and clock line to go high (or timeout)
    unsigned long waiting_since = millis();
    while (get_bus_state() != BusState::HOST_REQUEST_TO_SEND)
    {
      if ((millis() - waiting_since) > timeout_ms)
        return -1;
      delay(1);
    }

    portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;
    taskENTER_CRITICAL(&mux);

    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    golo(_ps2clk);
    delayMicroseconds(CLK_HALF_PERIOD_MICROS);
    gohi(_ps2clk);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

    while (bit < 0x0100)
    {
      if (digitalRead(_ps2data) == HIGH)
      {
        data = data | bit;
        calculated_parity = calculated_parity ^ 1;
      }
      else
      {
        calculated_parity = calculated_parity ^ 0;
      }

      bit = bit << 1;

      delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
      golo(_ps2clk);
      delayMicroseconds(CLK_HALF_PERIOD_MICROS);
      gohi(_ps2clk);
      delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    }
    // we do the delay at the end of the loop, so at this point we have
    // already done the delay for the parity bit

    // parity bit
    if (digitalRead(_ps2data) == HIGH)
    {
      received_parity = 1;
    }

    // stop bit
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    golo(_ps2clk);
    delayMicroseconds(CLK_HALF_PERIOD_MICROS);
    gohi(_ps2clk);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);

    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    golo(_ps2data);
    golo(_ps2clk);
    delayMicroseconds(CLK_HALF_PERIOD_MICROS);
    gohi(_ps2clk);
    delayMicroseconds(CLK_QUATER_PERIOD_MICROS);
    gohi(_ps2data);

    taskEXIT_CRITICAL(&mux);

    *value = data & 0x00FF;

    if (received_parity == calculated_parity)
    {
      return 0;
    }
    else
    {
      return -2;
    }
  }
  PS2dev::BusState PS2dev::get_bus_state()
  {
    if (digitalRead(_ps2clk) == LOW)
    {
      return BusState::COMMUNICATION_INHIBITED;
    }
    else if (digitalRead(_ps2data) == LOW)
    {
      return BusState::HOST_REQUEST_TO_SEND;
    }
    else
    {
      return BusState::IDLE;
    }
  }
  SemaphoreHandle_t PS2dev::get_bus_mutex_handle() { return _mutex_bus; }
  QueueHandle_t PS2dev::get_packet_queue_handle() { return _queue_packet; }
  int PS2dev::send_packet(PS2Packet *packet) { return (xQueueSend(_queue_packet, packet, 0) == pdTRUE) ? 0 : -1; }

  PS2Mouse::PS2Mouse(int clk, int data) : PS2dev(clk, data) {}
  void PS2Mouse::begin()
  {
    PS2dev::begin(DEFAULT_TASK_CORE_MOUSE);

    xSemaphoreTake(_mutex_bus, portMAX_DELAY);
    delay(200);
    write(0xAA);
    delayMicroseconds(BYTE_INTERVAL_MICROS);
    write(0x00);
    xSemaphoreGive(_mutex_bus);

    xTaskCreatePinnedToCore(_taskfn_poll_mouse_count, "PS2Mouse", 4096, this, _config_task_priority - 1, &_task_poll_mouse_count, DEFAULT_TASK_CORE_MOUSE);
  }
  int PS2Mouse::reply_to_host(uint8_t host_cmd)
  {
    uint8_t val;
    if (_mode == Mode::WRAP_MODE)
    {
      switch ((Command)host_cmd)
      {
      case Command::SET_WRAP_MODE: // set wrap mode
#if defined(_ESP32_PS2DEV_DEBUG_)
        ESP_LOGI(TAG, "PS2Mouse::reply_to_host: (WRAP_MODE) Set wrap mode command received");
#endif
        ack();
        reset_counter();
        break;
      case Command::RESET_WRAP_MODE: // reset wrap mode
#if defined(_ESP32_PS2DEV_DEBUG_)
        ESP_LOGI(TAG, "PS2Mouse::reply_to_host: (WRAP_MODE) Reset wrap mode command received");
#endif
        ack();
        reset_counter();
        _mode = _last_mode;
        break;
      default:
        write(host_cmd);
      }
      return 0;
    }

    switch ((Command)host_cmd)
    {
    case Command::RESET: // reset
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Reset command received");
#endif
      ack();
      // the while loop lets us wait for the host to be ready
      while (write(0xAA) != 0)
        delay(1);
      delayMicroseconds(BYTE_INTERVAL_MICROS);
      while (write(0x00) != 0)
        delay(1);
      _has_wheel = false;
      _has_4th_and_5th_buttons = false;
      _sample_rate = 100;
      _resolution = ResolutionCode::RES_4;
      _scale = Scale::ONE_ONE;
      _data_reporting_enabled = false;
      _mode = Mode::STREAM_MODE;
      reset_counter();
      break;
    case Command::RESEND: // resend
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Resend command received");
#endif
      ack();
      break;
    case Command::SET_DEFAULTS: // set defaults
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Set defaults command received");
#endif
      // enter stream mode
      ack();
      _sample_rate = 100;
      _resolution = ResolutionCode::RES_4;
      _scale = Scale::ONE_ONE;
      _data_reporting_enabled = false;
      _mode = Mode::STREAM_MODE;
      reset_counter();
      break;
    case Command::DISABLE_DATA_REPORTING: // disable data reporting
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Disable data reporting command received");
#endif
      ack();
      _data_reporting_enabled = false;
      reset_counter();
      break;
    case Command::ENABLE_DATA_REPORTING: // enable data reporting
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Enable data reporting command received");
#endif
      ack();
      _data_reporting_enabled = true;
      reset_counter();
      break;
    case Command::SET_SAMPLE_RATE: // set sample rate
      ack();
      if (read(&val) == 0)
      {
        switch (val)
        {
        case 10:
        case 20:
        case 40:
        case 60:
        case 80:
        case 100:
        case 200:
          _sample_rate = val;
          _last_sample_rate[0] = _last_sample_rate[1];
          _last_sample_rate[1] = _last_sample_rate[2];
          _last_sample_rate[2] = val;
#if defined(_ESP32_PS2DEV_DEBUG_)
          ESP_LOGI(TAG, "Set sample rate command received: %x", val);
          //_ESP32_PS2DEV_DEBUG_.println(val);
#endif
          ack();
          break;

        default:
          break;
        }
        // _min_report_interval_us = 1000000 / sample_rate;
        reset_counter();
      }
      break;
    case Command::GET_DEVICE_ID: // get device id
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Get device id command received");
#endif
      ack();
      if (_last_sample_rate[0] == 200 && _last_sample_rate[1] == 100 && _last_sample_rate[2] == 80)
      {
        write(0x03); // Intellimouse with wheel
#if defined(_ESP32_PS2DEV_DEBUG_)
        ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Act as Intellimouse with wheel.");
#endif
        _has_wheel = true;
      }
      else if (_last_sample_rate[0] == 200 && _last_sample_rate[1] == 200 && _last_sample_rate[2] == 80 && _has_wheel == true)
      {
        write(0x04); // Intellimouse with 4th and 5th buttons
#if defined(_ESP32_PS2DEV_DEBUG_)
        ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Act as Intellimouse with 4th and 5th buttons.");
#endif
        _has_4th_and_5th_buttons = true;
      }
      else
      {
        write(0x00); // Standard PS/2 mouse
#if defined(_ESP32_PS2DEV_DEBUG_)
        ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Act as standard PS/2 mouse.");
#endif
        _has_wheel = false;
        _has_4th_and_5th_buttons = false;
      }
      reset_counter();
      break;
    case Command::SET_REMOTE_MODE: // set remote mode
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Set remote mode command received");
#endif
      // ack();
      delayMicroseconds(BYTE_INTERVAL_MICROS);
      while (write(0xFA) != 0)
        delay(1);
      delayMicroseconds(BYTE_INTERVAL_MICROS);
      reset_counter();
      _mode = Mode::REMOTE_MODE;
      break;
    case Command::SET_WRAP_MODE: // set wrap mode
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Set wrap mode command received");
#endif
      ack();
      reset_counter();
      _last_mode = _mode;
      _mode = Mode::WRAP_MODE;
      break;
    case Command::RESET_WRAP_MODE: // reset wrap mode
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Reset wrap mode command received");
#endif
      ack();
      reset_counter();
      break;
    case Command::READ_DATA: // read data
#if defined(_ESP32_PS2DEV_DEBUG_)
                             // ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Read data command received"); //////////////////////////////////////////////////////////////
#endif
      ack();
      _report();
      reset_counter();
      break;
    case Command::SET_STREAM_MODE: // set stream mode
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Set stream mode command received");
#endif
      ack();
      reset_counter();
      break;
    case Command::STATUS_REQUEST: // status request
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Status request command received");
#endif
      ack();
      _send_status();
      break;
    case Command::SET_RESOLUTION: // set resolution
      ack();
      if (read(&val) == 0 && val <= 3)
      {
        _resolution = (ResolutionCode)val;
#if defined(_ESP32_PS2DEV_DEBUG_)
        ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Set resolution command received: %x", val);
        //_ESP32_PS2DEV_DEBUG_.println(val, HEX);
#endif
        ack();
        reset_counter();
      }
      break;
    case Command::SET_SCALING_2_1: // set scaling 2:1
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Set scaling 2:1 command received");
#endif
      ack();
      _scale = Scale::TWO_ONE;
      break;
    case Command::SET_SCALING_1_1: // set scaling 1:1
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Set scaling 1:1 command received");
#endif
      ack();
      _scale = Scale::ONE_ONE;
      break;
    default:
      delayMicroseconds(BYTE_INTERVAL_MICROS);
      while ((write(0xFE) != 0))
        delay(1);
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Mouse::reply_to_host: Unknown command received: %x", host_cmd);
      //_ESP32_PS2DEV_DEBUG_.println(host_cmd, HEX);
#endif
      break;
    }
    return 0;
  }
  bool PS2Mouse::has_wheel() { return _has_wheel; }
  bool PS2Mouse::has_4th_and_5th_buttons() { return _has_4th_and_5th_buttons; }
  bool PS2Mouse::data_reporting_enabled() { return _data_reporting_enabled; }
  void PS2Mouse::reset_counter()
  {
    _count_x = 0;
    _count_y = 0;
    _count_z = 0;
    _count_x_overflow = 0;
    _count_y_overflow = 0;
  }
  uint8_t PS2Mouse::get_sample_rate() { return _sample_rate; }
  void PS2Mouse::move(int16_t x, int16_t y, int8_t wheel)
  {
    _count_x += x;
    _count_y -= y; // We need to decrement, because USB HID inverts the vertical axis
    _count_z -= wheel;
    xTaskNotifyGive(_task_poll_mouse_count);
  }
  void PS2Mouse::press(Button button)
  {
    switch (button)
    {
    case Button::LEFT:
      _button_left = 1;
      break;
    case Button::RIGHT:
      _button_right = 1;
      break;
    case Button::MIDDLE:
      _button_middle = 1;
      break;
    case Button::BUTTON_4:
      _button_4th = 1;
      break;
    case Button::BUTTON_5:
      _button_5th = 1;
      break;
    default:
      break;
    }
    xTaskNotifyGive(_task_poll_mouse_count);
  }
  void PS2Mouse::release(Button button)
  {
    switch (button)
    {
    case Button::LEFT:
      _button_left = 0;
      break;
    case Button::RIGHT:
      _button_right = 0;
      break;
    case Button::MIDDLE:
      _button_middle = 0;
      break;
    case Button::BUTTON_4:
      _button_4th = 0;
      break;
    case Button::BUTTON_5:
      _button_5th = 0;
      break;
    default:
      break;
    }
    xTaskNotifyGive(_task_poll_mouse_count);
  }
  void PS2Mouse::click(Button button)
  {
    press(button);
    delay(MOUSE_CLICK_PRESSING_DURATION_MILLIS);
    release(button);
  }
  void PS2Mouse::_report()
  {
    PS2Packet packet;
    if (_scale == Scale::TWO_ONE)
    {
      int16_t *p[2] = {&_count_x, &_count_y};
      for (size_t i = 0; i < 2; i++)
      {
        bool positive = *p[i] >= 0;
        uint16_t abs_value = positive ? *p[i] : -*p[i];
        switch (abs_value)
        {
        case 1:
          abs_value = 1;
          break;
        case 2:
          abs_value = 1;
          break;
        case 3:
          abs_value = 3;
          break;
        case 4:
          abs_value = 6;
          break;
        case 5:
          abs_value = 9;
          break;
        default:
          abs_value *= 2;
          break;
        }
        if (!positive)
          *p[i] = -abs_value;
      }
    }
    if (_count_x > 255)
    {
      _count_x_overflow = 1;
      _count_x = 255;
    }
    else if (_count_x < -255)
    {
      _count_x_overflow = 1;
      _count_x = -255;
    }
    if (_count_y > 255)
    {
      _count_y_overflow = 1;
      _count_y = 255;
    }
    else if (_count_y < -255)
    {
      _count_y_overflow = 1;
      _count_y = -255;
    }
    if (_count_z > 7)
    {
      _count_z = 7;
    }
    else if (_count_z < -8)
    {
      _count_z = -8;
    }

    packet.len = 3 + _has_wheel;
    packet.data[0] = (_button_left) | ((_button_right) << 1) | ((_button_middle) << 2) | (1 << 3) | ((_count_x < 0) << 4) |
                     ((_count_y < 0) << 5) | (_count_x_overflow << 6) | (_count_y_overflow << 7);
    packet.data[1] = _count_x & 0xFF;
    packet.data[2] = _count_y & 0xFF;
    if (_has_wheel && !_has_4th_and_5th_buttons)
    {
      packet.data[3] = _count_z & 0xFF;
    }
    else if (_has_wheel && _has_4th_and_5th_buttons)
    {
      packet.data[3] = (_count_z & 0x0F) | ((_button_4th) << 4) | ((_button_5th) << 5);
    }

    send_packet(&packet);
    reset_counter();
  }
  void PS2Mouse::_send_status()
  {
    PS2Packet packet;
    packet.len = 3;
    bool mode = (_mode == Mode::REMOTE_MODE);
    packet.data[0] = (_button_right & 1) | ((_button_middle & 1) << 1) | ((_button_left & 1) << 2) | ((0) << 3) |
                     (((uint8_t)_scale & 1) << 4) | ((_data_reporting_enabled & 1) << 5) | ((mode & 1) << 6) | ((0) << 7);
    packet.data[1] = (uint8_t)_resolution;
    packet.data[2] = _sample_rate;
    send_packet(&packet);
  }

  PS2Keyboard::PS2Keyboard(int clk, int data) : PS2dev(clk, data) {}
  void PS2Keyboard::begin()
  {
    PS2dev::begin();

    xSemaphoreTake(_mutex_bus, portMAX_DELAY);
    delayMicroseconds(BYTE_INTERVAL_MICROS);
    delay(200);
    write(0xAA);
    xSemaphoreGive(_mutex_bus);
    apply_behavior_to_all(BEHAVIOR_TYP_MAKE_BREAK);
    update_typematic(_typematic_config);
  }
  void PS2Keyboard::apply_behavior_to_all(const KeyBehavior &behavior)
  {
    for (auto &entry : _key_behaviors)
      entry = behavior;
  }
  void PS2Keyboard::set_key_behavior(scancodes::Key key, const KeyBehavior &behavior)
  {
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
    int16_t mapped = HID_TO_PS2_LOOKUP[hid_code];
    if (mapped == INVALID_KEY)
      return false;
    return _key_behaviors[static_cast<size_t>(mapped)].typematic;
  }

  void PS2Keyboard::update_typematic(uint8_t config)
  {
    _typematic_config = config;
    uint8_t delay_index = (config >> 5) & 0x03;
    if (delay_index > 3)
      delay_index = 3;
    _typematic_delay_ms = DELAY_TABLE[delay_index];
    uint8_t rate_index = config & 0x1F;
    _typematic_cycle_ms = RATE_CYCLE_TABLE[rate_index];
  }
  void PS2Keyboard::configure_specific_key(uint8_t scan_code, const KeyBehavior &behavior)
  {
    if (_scan_code_set != 3)
    {
      return;
    }
    scancodes::Key key;
    if (key_from_set3_code(scan_code, key))
    {
      set_key_behavior(key, behavior);
    }
    else
    {
      ESP_LOGW(TAG, "Unknown Set 3 key code 0x%02x for make-only configuration", scan_code);
    }
  }
  bool PS2Keyboard::data_reporting_enabled() { return _data_reporting_enabled; }
  bool PS2Keyboard::is_scroll_lock_led_on() { return _led_scroll_lock; }
  bool PS2Keyboard::is_num_lock_led_on() { return _led_num_lock; }
  bool PS2Keyboard::is_caps_lock_led_on() { return _led_caps_lock; }
  int PS2Keyboard::reply_to_host(uint8_t host_cmd)
  {
    uint8_t val;
    switch ((Command)host_cmd)
    {
    case Command::RESET: // reset
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Reset command received");
#endif // _ESP32_PS2DEV_DEBUG_
       // the while loop lets us wait for the host to be ready
      _data_reporting_enabled = false;
      ack(); // ack() provides delay, some systems need it
      while (write((uint8_t)Command::BAT_SUCCESS) != 0)
        delay(1);
      _data_reporting_enabled = true; // some systems don't enable data reporting after issuing a RESET command, so we do it by default
      _scan_code_set = 2;
      apply_behavior_to_all(BEHAVIOR_TYP_MAKE_BREAK);
      break;
    case Command::RESEND: // resend
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Resend command received");
#endif // _ESP32_PS2DEV_DEBUG_
      ack();
      break;
    case Command::SET_DEFAULTS: // set defaults
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Set defaults command received");
#endif // _ESP32_PS2DEV_DEBUG_
       // enter stream mode
      ack();
      apply_behavior_to_all(BEHAVIOR_TYP_MAKE_BREAK);
      break;
    case Command::DISABLE_DATA_REPORTING: // disable data reporting
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Disable data reporting command received");
#endif // _ESP32_PS2DEV_DEBUG_
      _data_reporting_enabled = false;
      ack();
      break;
    case Command::ENABLE_DATA_REPORTING: // enable data reporting
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Enable data reporting command received");
#endif // _ESP32_PS2DEV_DEBUG_
      _data_reporting_enabled = true;
      ack();
      break;
    case Command::SET_TYPEMATIC_RATE: // set typematic rate
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Set typematic rate command received");
#endif // _ESP32_PS2DEV_DEBUG_
      ack();
      if (!read(&val))
      {
        ack();
        update_typematic(val);
      }
      break;
    case Command::GET_DEVICE_ID: // get device id
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Get device id command received");
#endif // _ESP32_PS2DEV_DEBUG_
      ack();
      while (write(0xAB) != 0)
        delay(1); // ensure ID gets writed, some hosts may be sensitive
      while (write(0x83) != 0)
        delay(1); // this is critical for combined ports (they decide mouse/kb on this)
      break;
    case Command::SET_SCAN_CODE_SET: // set scan code set
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Set scan code set command received");
#endif // _ESP32_PS2DEV_DEBUG_
      ack();
      if (!read(&val)) {
        ack();
        if (val == 0) {
          static const uint8_t resp[] = { 0x43, 0x41, 0x3F };
          delayMicroseconds(BYTE_INTERVAL_MICROS);
          write(resp[_scan_code_set]);
          delayMicroseconds(BYTE_INTERVAL_MICROS);
          break;
        } else if (val == 2 || val == 3) {
          _scan_code_set = val;
          apply_behavior_to_all(BEHAVIOR_TYP_MAKE_BREAK);
        }
      }
      break;
    case Command::ECHO: // echo
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Echo command received");
#endif // _ESP32_PS2DEV_DEBUG_
      delayMicroseconds(BYTE_INTERVAL_MICROS);
      write(0xEE);
      delayMicroseconds(BYTE_INTERVAL_MICROS);
      break;
    case Command::SET_RESET_LEDS: // set/reset LEDs
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Set/reset LEDs command received");
#endif // _ESP32_PS2DEV_DEBUG_
      delayMicroseconds(BYTE_INTERVAL_MICROS);
      while (write(0xFA) != 0)
        delay(1);
      delayMicroseconds(BYTE_INTERVAL_MICROS);
      if (!read(&val, 10))
      {
        delayMicroseconds(BYTE_INTERVAL_MICROS);
        while (write(0xFA) != 0)
          delay(1);
        delayMicroseconds(BYTE_INTERVAL_MICROS);
        _led_scroll_lock = ((val & KeyLed::KEYBOARD_LED_SCROLLLOCK) != 0);
        _led_num_lock = ((val & KeyLed::KEYBOARD_LED_NUMLOCK) != 0);
        _led_caps_lock = ((val & KeyLed::KEYBOARD_LED_CAPSLOCK) != 0);
        trigger_leds_callback(val);
      }
      return 1;
      break;
    case Command::SET_ALL_KEYS_TO_TYPEMATIC_AUTOREPEAT_ONLY: // Set all keys to typematic/autorepeat only (scancode set 3 only)
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Set all keys to typematic/autorepeat only");
#endif // _ESP32_PS2DEV_DEBUG_
      ack();
      if (_scan_code_set == 3)
      {
        apply_behavior_to_all(BEHAVIOR_TYP_ONLY);
      }
      break;
    case Command::SET_ALL_KEYS_TO_MAKE_RELEASE: // Set all keys to make/release (scancode set 3 only)
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Set all keys to make/release");
#endif // _ESP32_PS2DEV_DEBUG_
      ack();
      if (_scan_code_set == 3)
      {
        apply_behavior_to_all(BEHAVIOR_MAKE_BREAK);
      }
      break;
    case Command::SET_ALL_KEYS_TO_MAKE_ONLY: // Set all keys to make only (scancode set 3 only)
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Set all keys to make only");
#endif // _ESP32_PS2DEV_DEBUG_
      ack();
      if (_scan_code_set == 3)
      {
        apply_behavior_to_all(BEHAVIOR_MAKE_ONLY);
      }
      break;
    case Command::SET_ALL_KEYS_TO_TYPEMATIC_AUTOREPEAT_MAKE_RELEASE: // Set all keys to typematic/autorepeat/make/release (scancode set 3 only)
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Set all keys to typematic/autorepeat/make/release");
#endif // _ESP32_PS2DEV_DEBUG_
      ack();
      if (_scan_code_set == 3)
      {
        apply_behavior_to_all(BEHAVIOR_TYP_MAKE_BREAK);
      }
      break;
    case Command::SET_SPECIFIC_KEY_TO_TYPEMATIC_AUTOREPEAT_ONLY: // Set specific key to typematic/autorepeat only (scancode set 3 only)
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Set specific key to typematic/autorepeat only");
#endif // _ESP32_PS2DEV_DEBUG_
      ack();
      if (_scan_code_set == 3)
      {
        if (!read(&val))
        {
          ack();
          configure_specific_key(val, BEHAVIOR_TYP_ONLY);
        }
      }
      break;
    case Command::SET_SPECIFIC_KEY_TO_MAKE_RELEASE: // Set specific key to make/release (scancode set 3 only)
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Set specific key to make/release");
#endif // _ESP32_PS2DEV_DEBUG_
      ack();
      if (_scan_code_set == 3)
      {
        if (!read(&val))
        {
          ack();
          configure_specific_key(val, BEHAVIOR_MAKE_BREAK);
        }
      }
      break;
    case Command::SET_SPECIFIC_KEY_TO_MAKE_ONLY: // Set specific key to make only (scancode set 3 only)
#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Set specific key to make only");
#endif // _ESP32_PS2DEV_DEBUG_
      ack();
      if (_scan_code_set == 3)
      {
        if (!read(&val))
        {
          ack();
          configure_specific_key(val, BEHAVIOR_MAKE_ONLY);
        }
      }
      break;
    default:
      ack();
//#if defined(_ESP32_PS2DEV_DEBUG_)
      ESP_LOGI(TAG, "PS2Keyboard::reply_to_host: Unknown command received: %x", host_cmd);
      //_ESP32_PS2DEV_DEBUG_.println(host_cmd, HEX);
//#endif // _ESP32_PS2DEV_DEBUG_
      break;
    }

    return 0;
  }
  void PS2Keyboard::keydown(scancodes::Key key)
  {
    if (!_data_reporting_enabled)
      return;
    const auto &behavior = behavior_for_key(key);
    if (!behavior.make)
      return;
    PS2Packet packet;
    packet.len = (_scan_code_set == 3) ? scancodes::MAKE_SET3_CODES_LEN[key] : scancodes::MAKE_CODES_LEN[key];
    for (uint8_t i = 0; i < packet.len; i++)
    {
      packet.data[i] = (_scan_code_set == 3) ? scancodes::MAKE_SET3_CODES[key][i] : scancodes::MAKE_CODES[key][i];
    }
    send_packet(&packet);
  }
  void PS2Keyboard::keyup(scancodes::Key key)
  {
    if (!_data_reporting_enabled)
      return;
    const auto &behavior = behavior_for_key(key);
    if (!behavior.break_code)
      return;
    PS2Packet packet;
    packet.len = (_scan_code_set == 3) ? scancodes::BREAK_SET3_CODES_LEN[key] : scancodes::BREAK_CODES_LEN[key];
    for (uint8_t i = 0; i < packet.len; i++)
    {
      packet.data[i] = (_scan_code_set == 3) ? scancodes::BREAK_SET3_CODES[key][i] : scancodes::BREAK_CODES[key][i];
    }
    send_packet(&packet);
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
    for (auto key : keys)
    {
      keydown(key);
      stack.push(key);
      delay(10);
    }
    while (!stack.empty())
    {
      keyup(stack.top());
      stack.pop();
      delay(10);
    }
  }
  void PS2Keyboard::type(const char *str)
  {
    size_t i = 0;
    while (str[i] != '\0')
    {
      char c = str[i];
      scancodes::Key key;
      bool shift = false;
      switch (c)
      {
      case '\b':
        key = scancodes::Key::K_BACKSPACE;
        break;
      case '\t':
        key = scancodes::Key::K_TAB;
        break;
      case '\r':
      case '\n':
        key = scancodes::Key::K_RETURN;
        break;
      case ' ':
        key = scancodes::Key::K_SPACE;
        break;
      case '!':
        shift = true;
        key = scancodes::Key::K_1;
        break;
      case '\"':
        shift = true;
        key = scancodes::Key::K_QUOTE;
        break;
      case '#':
        shift = true;
        key = scancodes::Key::K_3;
        break;
      case '$':
        shift = true;
        key = scancodes::Key::K_4;
        break;
      case '&':
        shift = true;
        key = scancodes::Key::K_7;
        break;
      case '\'':
        key = scancodes::Key::K_QUOTE;
        break;
      case '(':
        shift = true;
        key = scancodes::Key::K_9;
        break;
      case ')':
        shift = true;
        key = scancodes::Key::K_0;
        break;
      case '*':
        shift = true;
        key = scancodes::Key::K_8;
        break;
      case '+':
        shift = true;
        key = scancodes::Key::K_EQUALS;
        break;
      case ',':
        key = scancodes::Key::K_COMMA;
        break;
      case '-':
        key = scancodes::Key::K_MINUS;
        break;
      case '.':
        key = scancodes::Key::K_PERIOD;
        break;
      case '/':
        key = scancodes::Key::K_SLASH;
        break;
      case '0':
        key = scancodes::Key::K_0;
        break;
      case '1':
        key = scancodes::Key::K_1;
        break;
      case '2':
        key = scancodes::Key::K_2;
        break;
      case '3':
        key = scancodes::Key::K_3;
        break;
      case '4':
        key = scancodes::Key::K_4;
        break;
      case '5':
        key = scancodes::Key::K_5;
        break;
      case '6':
        key = scancodes::Key::K_6;
        break;
      case '7':
        key = scancodes::Key::K_7;
        break;
      case '8':
        key = scancodes::Key::K_8;
        break;
      case '9':
        key = scancodes::Key::K_9;
        break;
      case ':':
        shift = true;
        key = scancodes::Key::K_SEMICOLON;
        break;
      case ';':
        key = scancodes::Key::K_SEMICOLON;
        break;
      case '<':
        shift = true;
        key = scancodes::Key::K_COMMA;
        break;
      case '=':
        key = scancodes::Key::K_EQUALS;
        break;
      case '>':
        shift = true;
        key = scancodes::Key::K_PERIOD;
        break;
      case '\?':
        shift = true;
        key = scancodes::Key::K_SLASH;
        break;
      case '@':
        shift = true;
        key = scancodes::Key::K_2;
        break;
      case '[':
        key = scancodes::Key::K_LEFTBRACKET;
        break;
      case '\\':
        key = scancodes::Key::K_BACKSLASH;
        break;
      case ']':
        key = scancodes::Key::K_RIGHTBRACKET;
        break;
      case '^':
        shift = true;
        key = scancodes::Key::K_6;
        break;
      case '_':
        shift = true;
        key = scancodes::Key::K_MINUS;
        break;
      case '`':
        key = scancodes::Key::K_BACKQUOTE;
        break;
      case 'a':
        key = scancodes::Key::K_A;
        break;
      case 'b':
        key = scancodes::Key::K_B;
        break;
      case 'c':
        key = scancodes::Key::K_C;
        break;
      case 'd':
        key = scancodes::Key::K_D;
        break;
      case 'e':
        key = scancodes::Key::K_E;
        break;
      case 'f':
        key = scancodes::Key::K_F;
        break;
      case 'g':
        key = scancodes::Key::K_G;
        break;
      case 'h':
        key = scancodes::Key::K_H;
        break;
      case 'i':
        key = scancodes::Key::K_I;
        break;
      case 'j':
        key = scancodes::Key::K_J;
        break;
      case 'k':
        key = scancodes::Key::K_K;
        break;
      case 'l':
        key = scancodes::Key::K_L;
        break;
      case 'm':
        key = scancodes::Key::K_M;
        break;
      case 'n':
        key = scancodes::Key::K_N;
        break;
      case 'o':
        key = scancodes::Key::K_O;
        break;
      case 'p':
        key = scancodes::Key::K_P;
        break;
      case 'q':
        key = scancodes::Key::K_Q;
        break;
      case 'r':
        key = scancodes::Key::K_R;
        break;
      case 's':
        key = scancodes::Key::K_S;
        break;
      case 't':
        key = scancodes::Key::K_T;
        break;
      case 'u':
        key = scancodes::Key::K_U;
        break;
      case 'v':
        key = scancodes::Key::K_V;
        break;
      case 'w':
        key = scancodes::Key::K_W;
        break;
      case 'x':
        key = scancodes::Key::K_X;
        break;
      case 'y':
        key = scancodes::Key::K_Y;
        break;
      case 'z':
        key = scancodes::Key::K_Z;
        break;
      case 'A':
        shift = true;
        key = scancodes::Key::K_A;
        break;
      case 'B':
        shift = true;
        key = scancodes::Key::K_B;
        break;
      case 'C':
        shift = true;
        key = scancodes::Key::K_C;
        break;
      case 'D':
        shift = true;
        key = scancodes::Key::K_D;
        break;
      case 'E':
        shift = true;
        key = scancodes::Key::K_E;
        break;
      case 'F':
        shift = true;
        key = scancodes::Key::K_F;
        break;
      case 'G':
        shift = true;
        key = scancodes::Key::K_G;
        break;
      case 'H':
        shift = true;
        key = scancodes::Key::K_H;
        break;
      case 'I':
        shift = true;
        key = scancodes::Key::K_I;
        break;
      case 'J':
        shift = true;
        key = scancodes::Key::K_J;
        break;
      case 'K':
        shift = true;
        key = scancodes::Key::K_K;
        break;
      case 'L':
        shift = true;
        key = scancodes::Key::K_L;
        break;
      case 'M':
        shift = true;
        key = scancodes::Key::K_M;
        break;
      case 'N':
        shift = true;
        key = scancodes::Key::K_N;
        break;
      case 'O':
        shift = true;
        key = scancodes::Key::K_O;
        break;
      case 'P':
        shift = true;
        key = scancodes::Key::K_P;
        break;
      case 'Q':
        shift = true;
        key = scancodes::Key::K_Q;
        break;
      case 'R':
        shift = true;
        key = scancodes::Key::K_R;
        break;
      case 'S':
        shift = true;
        key = scancodes::Key::K_S;
        break;
      case 'T':
        shift = true;
        key = scancodes::Key::K_T;
        break;
      case 'U':
        shift = true;
        key = scancodes::Key::K_U;
        break;
      case 'V':
        shift = true;
        key = scancodes::Key::K_V;
        break;
      case 'W':
        shift = true;
        key = scancodes::Key::K_W;
        break;
      case 'X':
        shift = true;
        key = scancodes::Key::K_X;
        break;
      case 'Y':
        shift = true;
        key = scancodes::Key::K_Y;
        break;
      case 'Z':
        shift = true;
        key = scancodes::Key::K_Z;
        break;

      default:
        i++;
        continue;
        break;
      }
      if (shift)
      {
        keydown(scancodes::Key::K_LSHIFT);
        delay(10);
        type(key);
        delay(10);
        keyup(scancodes::Key::K_LSHIFT);
      }
      else
      {
        type(key);
      }
      i++;
    }
  }

  void _taskfn_process_host_request(void *arg)
  {
    PS2dev *ps2dev = (PS2dev *)arg;
    while (true)
    {
      xSemaphoreTake(ps2dev->get_bus_mutex_handle(), portMAX_DELAY);
      if (ps2dev->get_bus_state() == PS2dev::BusState::HOST_REQUEST_TO_SEND)
      {
        uint8_t host_cmd;
        if (ps2dev->read(&host_cmd) == 0)
        {
          ps2dev->reply_to_host(host_cmd);
        }
      }
      xSemaphoreGive(ps2dev->get_bus_mutex_handle());
      delay(INTERVAL_CHECKING_HOST_SEND_REQUEST_MILLIS);
    }
    vTaskDelete(NULL);
  }
  void _taskfn_send_packet(void *arg)
  {
    PS2dev *ps2dev = (PS2dev *)arg;
    while (true)
    {
      PS2Packet packet;
      if (xQueueReceive(ps2dev->get_packet_queue_handle(), &packet, portMAX_DELAY) == pdTRUE)
      {
        xSemaphoreTake(ps2dev->get_bus_mutex_handle(), portMAX_DELAY);
        delayMicroseconds(BYTE_INTERVAL_MICROS);
        for (int i = 0; i < packet.len; i++)
        {
          ps2dev->write_wait_idle(packet.data[i]);
          delayMicroseconds(BYTE_INTERVAL_MICROS);
        }
        xSemaphoreGive(ps2dev->get_bus_mutex_handle());
      }
    }
    vTaskDelete(NULL);
  }
  void _taskfn_poll_mouse_count(void *arg)
  {
    PS2Mouse *ps2mouse = (PS2Mouse *)arg;
    while (true)
    {
      xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
      if (ps2mouse->data_reporting_enabled())
      {
        ps2mouse->_report();
      }
      delay(1000 / ps2mouse->get_sample_rate());
    }
    vTaskDelete(NULL);
  }

  void PS2Keyboard::keyHid_send(uint8_t btkey, bool keyDown)
  {
    int16_t mapped = HID_TO_PS2_LOOKUP[btkey];
    if (mapped == INVALID_KEY)
    {
      return;
    }

    scancodes::Key key = static_cast<scancodes::Key>(mapped);
    if (keyDown)
      keydown(key);
    else
      keyup(key);
  }

  void PS2Keyboard::keyHid_send_CCONTROL(uint16_t btkey, bool keyDown)
  {
    scancodes::Key key;
    if (!map_consumer_usage(btkey, key))
    {
      return;
    }

    if (keyDown)
      keydown(key);
    else
      keyup(key);
  }

} // namespace esp32_ps2dev
