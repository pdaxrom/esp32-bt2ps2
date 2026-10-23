#pragma once
#include "hid_report.hpp"
#include <tuple>

namespace input {
struct Source {
    uintptr_t device = 0;
    uint8_t map = 0, report = 0;
    bool operator<(const Source &other) const {
        return std::tie(device, map, report) < std::tie(other.device, other.map, other.report);
    }
};
struct Snapshot {
    std::bitset<256> keys;
    std::array<uint16_t, hid::MAX_CONSUMER_KEYS> consumer{};
    uint8_t buttons = 0;
    // Cumulative relative movement survives snapshot queue resynchronization.
    int64_t x = 0, y = 0, wheel = 0;
};
// Owned by the HID callback task. Consumers receive copies, never shared mutable state.
class State {
public:
    bool update(Source source, const hid::Input &report);
    void disconnect(uintptr_t device);
    const Snapshot &snapshot() const { return snapshot_; }
private:
    bool aggregate(Snapshot &out) const;
    std::map<Source, hid::Input> reports_;
    Snapshot snapshot_;
};
} // namespace input
