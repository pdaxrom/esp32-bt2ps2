#include "hid_report.hpp"
#include <algorithm>
#include <limits>

namespace hid {
namespace {
constexpr size_t MAX_REPORT_BITS = 8192;
constexpr size_t MAX_FIELDS = 256;
constexpr size_t MAX_USAGES = 1024;
struct Globals {
    uint16_t page = 0;
    uint32_t size = 0, count = 0;
    uint8_t id = 0;
    int64_t minimum = 0, maximum = 0;
};
struct Locals {
    std::vector<UsageRange> usages;
    bool pending_minimum = false;
    size_t range_index = 0;
};
int32_t signed_value(uint32_t value, unsigned bytes)
{
    if (bytes == 0) return 0;
    if (bytes < 4 && (value & (uint32_t{1} << (bytes * 8 - 1))))
        value |= ~((uint32_t{1} << (bytes * 8)) - 1);
    return static_cast<int32_t>(value);
}
uint32_t usage(uint32_t value, unsigned bytes, uint16_t page)
{
    return bytes == 4 ? value : (uint32_t{page} << 16) | value;
}
bool add_consumer(Input &input, uint16_t value)
{
    if (!value) return true;
    for (auto &entry : input.consumer) {
        if (entry == value) return true;
        if (!entry) { entry = value; return true; }
    }
    return false;
}
} // namespace

uint32_t Field::usage_at(size_t index) const
{
    for (const auto &range : usages) {
        const size_t count = size_t{range.last - range.first} + 1;
        if (index < count) return range.first + index;
        index -= count;
    }
    return usages.empty() ? 0 : usages.back().last;
}
size_t Field::usage_count() const
{
    size_t count = 0;
    for (const auto &range : usages) count += size_t{range.last - range.first} + 1;
    return count;
}

bool parse(const uint8_t *data, size_t length, Reports &out)
{
    if (!data || !length) return false;
    Reports result;
    Globals globals;
    Locals locals;
    std::vector<Globals> stack;
    size_t depth = 0, field_count = 0;
    bool mouse_scope = false;
    std::vector<bool> collections;
    bool numbered = false, unnumbered_input = false;
    for (size_t index = 0; index < length;) {
        uint8_t prefix = data[index++];
        if (prefix == 0xFE) return false; // Long items aren't supported.
        unsigned bytes = prefix & 3;
        if (bytes == 3) bytes = 4;
        if (bytes > length - index) return false;
        uint32_t value = 0;
        for (unsigned i = 0; i < bytes; ++i) value |= uint32_t{data[index++]} << (8 * i);
        unsigned type = (prefix >> 2) & 3, tag = prefix >> 4;
        if (type == 1) {
            switch (tag) {
            case 0: if (value > 0xFFFF) return false; globals.page = value; break;
            case 1: globals.minimum = signed_value(value, bytes); break;
            case 2: globals.maximum = globals.minimum < 0 ? int64_t{signed_value(value, bytes)} : int64_t{value}; break;
            case 7: globals.size = value; break;
            case 8:
                if (!value || value > 255 || unnumbered_input) return false;
                globals.id = value; numbered = true; break;
            case 9: globals.count = value; break;
            case 10: if (stack.size() >= 16) return false; stack.push_back(globals); break;
            case 11: if (stack.empty()) return false; globals = stack.back(); stack.pop_back(); break;
            default: break; // Physical range, units and exponents don't affect input decoding.
            }
        } else if (type == 2) {
            switch (tag) {
            case 0:
                if (locals.usages.size() >= MAX_USAGES) return false;
                locals.usages.push_back({usage(value, bytes, globals.page), usage(value, bytes, globals.page)}); break;
            case 1:
                if (locals.pending_minimum || locals.usages.size() >= MAX_USAGES) return false;
                locals.range_index = locals.usages.size();
                locals.usages.push_back({usage(value, bytes, globals.page), usage(value, bytes, globals.page)});
                locals.pending_minimum = true; break;
            case 2: {
                if (!locals.pending_minimum) return false;
                auto &range = locals.usages[locals.range_index];
                range.last = usage(value, bytes, globals.page);
                if (range.last < range.first || (range.last >> 16) != (range.first >> 16)) return false;
                locals.pending_minimum = false; break;
            }
            case 10: return false; // Delimiter alternative usage sets are unsupported.
            default: break; // Designator and String items do not affect input values.
            }
        } else if (type == 0) {
            if (tag == 8) { // Input; output/feature items have independent offsets.
                if (!globals.size || !globals.count || (numbered && globals.id == 0)) return false;
                if (globals.id == 0) unnumbered_input = true;
                const uint64_t bits = uint64_t{globals.size} * globals.count;
                auto &report = result[globals.id];
                if (bits > MAX_REPORT_BITS - report.bits || ++field_count > MAX_FIELDS) return false;
                if (locals.pending_minimum) return false;
                const bool relevant = std::any_of(locals.usages.begin(), locals.usages.end(), [&](const auto &range) {
                    const auto page = range.first >> 16;
                    return page == 7 || page == 0x0C || (mouse_scope && (page == 1 || page == 9));
                });
                if (!(value & 1) && relevant) { // Constants/vendor fields retain their offset without decoding.
                    if (globals.size > 32 || globals.maximum < globals.minimum || (value & 0x100)) return false;
                    Field field;
                    field.offset = report.bits;
                    field.size = globals.size; field.count = globals.count; field.flags = value;
                    field.logical_min = globals.minimum; field.logical_max = globals.maximum;
                    field.usages = locals.usages;
                    field.mouse = mouse_scope;
                    report.fields.push_back(std::move(field));
                }
                report.bits += bits;
            } else if (tag == 10) {
                if (++depth > 32) return false;
                collections.push_back(mouse_scope);
                if (value == 1) // Application collections define the device role.
                    mouse_scope = !locals.usages.empty() && locals.usages.front().first == 0x00010002;
            } else if (tag == 12) {
                if (!depth) return false;
                --depth;
                mouse_scope = collections.back(); collections.pop_back();
            } else if (tag != 9 && tag != 11) return false;
            locals = {}; // HID local items expire after every Main item.
        } else return false;
    }
    if (depth || !stack.empty() || result.empty()) return false;
    out = std::move(result);
    return true;
}

bool read_bits(const uint8_t *data, size_t length, size_t offset, unsigned width, uint32_t &out)
{
    if (!data || !width || width > 32 || length > std::numeric_limits<size_t>::max() / 8) return false;
    const size_t bits = length * 8;
    if (offset > bits || width > bits - offset) return false;
    out = 0;
    for (unsigned bit = 0; bit < width; ++bit)
        out |= uint32_t{(data[(offset + bit) / 8] >> ((offset + bit) % 8)) & 1u} << bit;
    return true;
}

bool decode(const Report &report, const uint8_t *data, size_t length, Input &out)
{
    if (!data || length < (report.bits + 7) / 8) return false;
    Input result;
    for (const auto &field : report.fields) {
        for (size_t i = 0; i < field.count; ++i) {
            uint32_t raw;
            if (!read_bits(data, length, field.offset + i * field.size, field.size, raw)) return false;
            int64_t value = raw;
            if (field.logical_min < 0) {
                uint32_t extended = raw;
                if (field.size < 32 && (raw & (uint32_t{1} << (field.size - 1))))
                    extended |= ~uint32_t{0} << field.size;
                value = static_cast<int32_t>(extended);
            }
            uint32_t code;
            if (field.flags & 2) code = field.usage_at(i); // Variable bitmap/axis.
            else { // Array values index the local usage list, relative to Logical Minimum.
                if (value < field.logical_min || value > field.logical_max) continue; // Null state.
                const size_t index = static_cast<size_t>(int64_t{value} - field.logical_min);
                const size_t available = field.usage_count();
                if (index >= available) continue;
                code = field.usage_at(index);
            }
            const uint16_t page = code >> 16, id = code & 0xFFFF;
            const bool active = !(field.flags & 2) || raw != 0;
            if (page == 7 && active) {
                if (id >= 1 && id <= 3) return false; // Rollover/undefined/error: keep the previous state.
                if (id < result.keys.size() && id) result.keys.set(id);
            } else if (page == 0x0C && active) {
                if (!add_consumer(result, id)) return false;
            } else if (page == 9 && field.mouse && active && id >= 1 && id <= 5) {
                result.buttons |= 1u << (id - 1);
            } else if (page == 1 && field.mouse && (field.flags & 4)) { // PS/2 uses relative axes only.
                const auto axis = std::clamp<int64_t>(value, INT32_MIN, INT32_MAX);
                if (id == 0x30) result.x = axis;
                else if (id == 0x31) result.y = axis;
                else if (id == 0x38) result.wheel = axis;
            }
        }
    }
    out = result;
    return true;
}
} // namespace hid
