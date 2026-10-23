#pragma once
#include "ps2_packet.hpp"
#include <algorithm>

namespace esp32_ps2dev {
inline int64_t scale_mouse_axis(int32_t count, bool two_one)
{
    int64_t value = count;
    if (!two_one) return value;
    const bool negative = value < 0;
    value = negative ? -value : value;
    constexpr int values[] = {0, 1, 1, 3, 6, 9};
    value = value < 6 ? values[value] : value * 2;
    return negative ? -value : value;
}
struct MouseReport {
    PS2Packet packet;
    int8_t wheel_consumed = 0;
};
inline MouseReport encode_mouse_report(int32_t x, int32_t y, int32_t wheel, uint8_t buttons,
                                      bool two_one, bool has_wheel, bool extra_buttons)
{
    MouseReport report;
    const int64_t scaled_x = scale_mouse_axis(x, two_one), scaled_y = scale_mouse_axis(y, two_one);
    const int16_t clipped_x = std::clamp<int64_t>(scaled_x, -256, 255);
    const int16_t clipped_y = std::clamp<int64_t>(scaled_y, -256, 255);
    report.wheel_consumed = has_wheel ? std::clamp<int32_t>(wheel, -8, 7) : 0;
    auto &packet = report.packet;
    packet.len = 3 + has_wheel;
    packet.data[0] = (buttons & 7) | 8 | ((clipped_x < 0) << 4) | ((clipped_y < 0) << 5) |
        ((scaled_x < -256 || scaled_x > 255) << 6) | ((scaled_y < -256 || scaled_y > 255) << 7);
    packet.data[1] = clipped_x & 0xFF; packet.data[2] = clipped_y & 0xFF;
    if (has_wheel) packet.data[3] = extra_buttons
        ? (report.wheel_consumed & 0x0F) | ((buttons & 0x18) << 1) : uint8_t(report.wheel_consumed);
    return report;
}
}
