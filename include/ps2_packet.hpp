#pragma once
#include <cstdint>
namespace esp32_ps2dev {
struct PS2Packet {
    uint8_t len = 0;
    uint8_t data[16]{};
    uint32_t generation = 0; // Queue metadata, never transmitted on the wire.
};
}
