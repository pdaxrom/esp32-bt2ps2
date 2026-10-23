#pragma once
#include <cstdint>
namespace esp32_ps2dev {
void delay(uint32_t ms);
void delayMicroseconds(uint32_t us);
unsigned long millis();
unsigned long micros();
}
