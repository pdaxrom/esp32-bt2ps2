#include "hid_report.hpp"
#include "input_state.hpp"
#include <cassert>
#include <cstdio>
#include <random>

namespace {
const uint8_t BOOT[] = {
    0x05,1,0x09,6,0xA1,1,0x05,7,0x19,0xE0,0x29,0xE7,0x15,0,0x25,1,
    0x75,1,0x95,8,0x81,2,0x75,8,0x95,1,0x81,1,
    0x95,5,0x75,1,0x05,8,0x19,1,0x29,5,0x91,2,0x95,1,0x75,3,0x91,1,
    0x95,6,0x75,8,0x15,0,0x25,0x65,0x05,7,0x19,0,0x29,0x65,0x81,0,0xC0
};
void boot_keyboard()
{
    hid::Reports reports;
    assert(hid::parse(BOOT, sizeof(BOOT), reports));
    assert(reports.at(0).bits == 64);
    const uint8_t data[] = {2,0,4,0,5,0,0,0}; // Shift, A and B, including a hole.
    hid::Input input;
    assert(hid::decode(reports.at(0), data, sizeof(data), input));
    assert(input.keys.count() == 3 && input.keys[0xE1] && input.keys[4] && input.keys[5]);
    assert(!hid::decode(reports.at(0), data, sizeof(data)-1, input));
    uint8_t rollover[] = {0,0,1,1,1,1,1,1};
    assert(!hid::decode(reports.at(0), rollover, sizeof(rollover), input));
    assert(input.keys[4]); // Failure doesn't overwrite the last valid result.
    for (size_t length = 0; length < sizeof(BOOT); ++length) {
        hid::Reports partial;
        hid::parse(BOOT, length, partial); // Every truncated item is safe to inspect.
    }
}
void consumer_and_mixed_fields()
{
    const uint8_t desc[] = {
        0x05,0x0C,0x09,1,0xA1,1,0x85,3,
        0x15,0,0x25,1,0x19,0xE9,0x29,0xEA,0x75,1,0x95,2,0x81,2,
        0x75,1,0x95,1,0x81,1, // One padding bit, array starts at bit 3.
        0x19,0,0x2A,0xFF,0,0x25,0xFF,0x75,8,0x95,1,0x81,0,
        0x75,1,0x95,5,0x81,1,0xC0
    };
    hid::Reports reports;
    assert(hid::parse(desc, sizeof(desc), reports));
    const uint8_t data[] = {uint8_t(1 | (0xCD << 3)), uint8_t(0xCD >> 5)};
    hid::Input input;
    assert(hid::decode(reports.at(3), data, sizeof(data), input));
    assert(input.consumer[0] == 0xE9 && input.consumer[1] == 0xCD && input.consumer[2] == 0);
    // Variable fields map usage per element, not per bit of a multi-bit value.
    const uint8_t multibit[] = {0x05,0x0C,0x09,1,0xA1,1,0x09,0xE9,0x09,0xEA,
        0x15,0,0x25,3,0x75,2,0x95,2,0x81,2,0xC0};
    assert(hid::parse(multibit, sizeof(multibit), reports));
    uint8_t value = 0x0C;
    assert(hid::decode(reports.at(0), &value, 1, input));
    assert(input.consumer[0] == 0xEA && input.consumer[1] == 0);
}
void mouse_and_bit_boundaries()
{
    const uint8_t desc[] = {0x05,1,0x09,2,0xA1,1,0x05,9,0x19,1,0x29,3,
        0x15,0,0x25,1,0x75,1,0x95,3,0x81,2,0x75,1,0x95,2,0x81,1,
        0x05,1,0x09,0x30,0x09,0x31,0x16,0,0x80,0x26,0xFF,0x7F,
        0x75,16,0x95,2,0x81,6,0xC0};
    hid::Reports reports;
    assert(hid::parse(desc, sizeof(desc), reports));
    uint8_t data[5] = {};
    // Three buttons plus padding; both signed 16-bit axes cross byte boundaries.
    data[0] = 5;
    const auto put = [&](size_t offset, uint16_t value) {
        for (unsigned i=0; i<16; ++i) data[(offset+i)/8] |= ((value>>i)&1) << ((offset+i)%8);
    };
    put(5, uint16_t(-12345)); put(21, 23456);
    hid::Input input;
    assert(hid::decode(reports.at(0), data, sizeof(data), input));
    assert(input.buttons == 5 && input.x == -12345 && input.y == 23456);
    uint32_t bits;
    const uint8_t previous_bug[] = {0,1};
    assert(hid::read_bits(previous_bug, 2, 1, 8, bits) && bits == 128);
    assert(!hid::read_bits(previous_bug, 1, 1, 8, bits));
    assert(!hid::read_bits(data, sizeof(data), 0, 0, bits));
    assert(!hid::read_bits(data, sizeof(data), 0, 33, bits));
}
void nkro_and_non_mouse_buttons()
{
    const uint8_t nkro[] = {0x05,7,0x19,4,0x29,0x0B,0x15,0,0x25,1,0x75,1,0x95,8,0x81,2};
    hid::Reports reports;
    assert(hid::parse(nkro, sizeof(nkro), reports));
    uint8_t value = 0xA5;
    hid::Input decoded;
    assert(hid::decode(reports.at(0), &value, 1, decoded));
    assert(decoded.keys.count() == 4 && decoded.keys[4] && decoded.keys[6] && decoded.keys[9] && decoded.keys[11]);
    const uint8_t consumer_buttons[] = {0x05,0x0C,0x09,1,0xA1,1,0x05,9,0x19,1,0x29,3,
        0x15,0,0x25,1,0x75,1,0x95,3,0x81,2,0xC0};
    assert(hid::parse(consumer_buttons, sizeof(consumer_buttons), reports));
    value=7;
    assert(hid::decode(reports.at(0), &value, 1, decoded));
    assert(decoded.buttons==0); // Button usages outside a mouse collection aren't mouse buttons.
}
void parser_context_and_ids()
{
    const uint8_t desc[] = {0x05,7,0x85,1,0x15,0,0x25,1,0x75,1,0x95,1,
        0x09,4,0x81,2,0xA4,0x85,2,0x09,5,0x81,2,0xB4,0x09,6,0x81,2};
    hid::Reports first, second;
    assert(hid::parse(desc, sizeof(desc), first));
    assert(first.at(1).bits == 2 && first.at(2).bits == 1);
    assert(hid::parse(BOOT, sizeof(BOOT), second));
    assert(second.size() == 1 && second.count(0));
    const uint8_t pop[] = {0xB4};
    assert(!hid::parse(pop, sizeof(pop), second));
    assert(second.at(0).bits == 64); // Failed parse leaves caller's map intact.
    const uint8_t invalid_range[] = {0x05,7,0x19,9,0x29,4,0x75,1,0x95,1,0x81,2};
    assert(!hid::parse(invalid_range, sizeof(invalid_range), second));
}
void multiple_devices_and_reports()
{
    input::State state;
    hid::Input a, b, c;
    a.keys.set(4); a.keys.set(0xE1); a.buttons = 1; a.x = 10; a.consumer[0] = 0xCD;
    b.keys.set(4); b.keys.set(5); b.buttons = 2; b.x = -3;
    assert(state.update({1,0,1}, a));
    assert(state.update({2,0,1}, b));
    assert(state.snapshot().keys.count() == 3 && state.snapshot().buttons == 3 && state.snapshot().x == 7);
    // Same report ID in a different map must have independent held state.
    c.keys.set(6);
    assert(state.update({1,1,1}, c));
    assert(state.update({1,0,1}, {}));
    assert(state.snapshot().keys[4] && !state.snapshot().keys[0xE1] && state.snapshot().keys[6]);
    state.disconnect(2);
    assert(!state.snapshot().keys[4] && !state.snapshot().keys[5] && state.snapshot().keys[6]);
    assert(state.snapshot().x == 7 && state.snapshot().buttons == 0);
    state.disconnect(1);
    assert(state.snapshot().keys.none() && state.snapshot().consumer[0] == 0);
}
void composite_descriptor_and_usage_order()
{
    // A vendor Feature of 64 bits must not disable the keyboard input in the same map.
    std::vector<uint8_t> desc(std::begin(BOOT), std::end(BOOT));
    const uint8_t feature[] = {0x06,0,0xFF,0x09,1,0x75,64,0x95,1,0xB1,2};
    desc.insert(desc.end(), std::begin(feature), std::end(feature));
    hid::Reports reports;
    assert(hid::parse(desc.data(), desc.size(), reports));
    assert(reports.at(0).bits == 64);
    // Ranges occupy their declared position among explicit usages, including multiple ranges.
    const uint8_t ordered[] = {0x05,7,0x19,4,0x29,5,0x09,6,0x19,7,0x29,8,
        0x15,0,0x25,1,0x75,1,0x95,5,0x81,2};
    assert(hid::parse(ordered, sizeof(ordered), reports));
    uint8_t value = 0x09;
    hid::Input input;
    assert(hid::decode(reports.at(0), &value, 1, input));
    assert(input.keys.count() == 2 && input.keys[4] && input.keys[7]);
    // Logical Maximum is unsigned when Logical Minimum is non-negative.
    const uint8_t unsigned32[] = {0x05,7,0x19,0,0x29,0x65,0x15,0,
        0x27,0xFF,0xFF,0xFF,0xFF,0x75,32,0x95,1,0x81,0};
    assert(hid::parse(unsigned32, sizeof(unsigned32), reports));
    const uint8_t key[] = {4,0,0,0};
    assert(hid::decode(reports.at(0), key, sizeof(key), input) && input.keys[4]);
    const uint8_t array[] = {0x05,7,0x19,4,0x29,5,0x09,6,0x19,7,0x29,8,
        0x15,5,0x25,9,0x75,8,0x95,1,0x81,0};
    assert(hid::parse(array, sizeof(array), reports));
    value = 7;
    assert(hid::decode(reports.at(0), &value, 1, input) && input.keys.count() == 1 && input.keys[6]);
    const uint8_t vendor[] = {0x06,0,0xFF,0x09,1,0x75,64,0x95,1,0x81,2,
        0x05,7,0x09,4,0x15,0,0x25,1,0x75,1,0x81,2};
    assert(hid::parse(vendor, sizeof(vendor), reports) && reports.at(0).bits == 65);
    const uint8_t mixed[] = {0,0,0,0,0,0,0,0,1};
    assert(hid::decode(reports.at(0), mixed, sizeof(mixed), input) && input.keys[4]);
    const uint8_t signed32[] = {0x05,1,0x09,2,0xA1,1,0x09,0x30,0x09,0x31,
        0x17,0,0,0,0x80,0x27,0xFF,0xFF,0xFF,0x7F,0x75,32,0x95,2,0x81,6,0xC0};
    assert(hid::parse(signed32, sizeof(signed32), reports));
    const uint8_t axes[] = {0,0,0,0x80,0xFF,0xFF,0xFF,0x7F};
    assert(hid::decode(reports.at(0), axes, sizeof(axes), input) && input.x == INT32_MIN && input.y == INT32_MAX);
}
void consumer_capacity_is_transactional()
{
    input::State state;
    hid::Input first, extra;
    for (size_t i = 0; i < first.consumer.size(); ++i) first.consumer[i] = i + 1;
    assert(state.update({1,0,0}, first));
    extra.consumer[0] = 33; extra.x = 10;
    assert(!state.update({2,0,0}, extra));
    assert(state.snapshot().consumer.back() == 32 && state.snapshot().x == 0);
    state.disconnect(2); assert(state.snapshot().consumer.back() == 32);
    const uint8_t desc[] = {0x05,0x0C,0x19,1,0x29,33,0x15,0,0x25,1,0x75,1,0x95,33,0x81,2};
    hid::Reports reports;
    assert(hid::parse(desc, sizeof(desc), reports));
    const uint8_t data[] = {0xFF,0xFF,0xFF,0xFF,1};
    hid::Input decoded; decoded.x = 42;
    assert(!hid::decode(reports.at(0), data, sizeof(data), decoded) && decoded.x == 42);
}
void fuzz_boundaries()
{
    std::mt19937 random(0xB72);
    for (unsigned trial = 0; trial < 20000; ++trial) {
        std::vector<uint8_t> data(random()%128);
        for (auto &byte : data) byte = random();
        hid::Reports reports;
        if (hid::parse(data.data(), data.size(), reports)) {
            for (const auto &report : reports) {
                hid::Input decoded;
                hid::decode(report.second, data.data(), data.size(), decoded);
            }
        }
        uint32_t value;
        hid::read_bits(data.data(), data.size(), random()%1100, random()%35, value);
    }
}
}
int main()
{
    boot_keyboard(); consumer_and_mixed_fields(); mouse_and_bit_boundaries();
    parser_context_and_ids(); nkro_and_non_mouse_buttons(); multiple_devices_and_reports();
    composite_descriptor_and_usage_order(); consumer_capacity_is_transactional(); fuzz_boundaries();
    puts("HID parsing, decoding, aggregation and 20000 fuzz cases passed");
}
