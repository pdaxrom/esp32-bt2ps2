#pragma once

#include <array>
#include <bitset>
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>

// Pure HID parsing and decoding. Buffers exclude the Report ID, as in ESP-IDF input events.
namespace hid {
constexpr size_t MAX_CONSUMER_KEYS = 32;
struct Input {
    std::bitset<256> keys;
    std::array<uint16_t, MAX_CONSUMER_KEYS> consumer{};
    uint8_t buttons = 0;
    int32_t x = 0, y = 0, wheel = 0;
};

struct UsageRange { uint32_t first, last; };
struct Field {
    size_t offset = 0;
    uint16_t size = 0, count = 0;
    uint32_t flags = 0;
    int64_t logical_min = 0, logical_max = 0;
    std::vector<UsageRange> usages; // Ordered single usages and ranges, combined Page/ID.
    bool mouse = false;
    uint32_t usage_at(size_t index) const;
    size_t usage_count() const;
};
struct Report {
    size_t bits = 0;
    std::vector<Field> fields;
};
using Reports = std::map<uint8_t, Report>;

// Reject unsupported/malformed descriptors transactionally; never publish a partial map.
bool parse(const uint8_t *data, size_t length, Reports &out);
bool read_bits(const uint8_t *data, size_t length, size_t offset, unsigned width, uint32_t &out);
bool decode(const Report &report, const uint8_t *data, size_t length, Input &out);
} // namespace hid
