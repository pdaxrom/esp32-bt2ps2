#include "input_bridge.hpp"
#include <algorithm>

namespace {
bool contains(const std::array<uint16_t, hid::MAX_CONSUMER_KEYS> &keys, uint16_t key)
{
    return std::find(keys.begin(), keys.end(), key) != keys.end();
}
}
void InputBridge::accept(const input::Snapshot &snapshot)
{
    target_ = snapshot;
    // Different HID usages can produce the same PS/2 key. Merge their owners before diffing.
    target_.keys.reset();
    keyboard_owners_.reset();
    for (size_t key = 1; key < snapshot.keys.size(); ++key)
        if (snapshot.keys[key]) {
            const auto mapped = output_.canonical_key(key);
            target_.keys.set(mapped); keyboard_owners_.set(mapped);
        }
    for (auto &key : target_.consumer) {
        if (auto mapped = output_.consumer_key(key)) { target_.keys.set(mapped); key = 0; }
    }
    // Mouse movement is cumulative in the input snapshots. Feed the difference once.
    output_.mouse(snapshot.x - applied_.x, snapshot.y - applied_.y, snapshot.wheel - applied_.wheel, snapshot.buttons);
    applied_.x = snapshot.x; applied_.y = snapshot.y; applied_.wheel = snapshot.wheel;
    applied_.buttons = snapshot.buttons;
}
void InputBridge::tick(int64_t now_ms)
{
    InputOutput::KeyUpdate update(output_);
    const auto revision = output_.revision();
    if (revision_ != revision) {
        revision_ = revision;
        applied_.keys.reset(); applied_.consumer.fill(0);
        repeat_key_ = 0;
    }
    const auto typematic = output_.typematic();
    if (repeat_key_ && !keyboard_owners_[repeat_key_]) repeat_key_ = 0;
    // Releases first. Only advance the applied state after the packet is queued.
    for (size_t key = 1; key < target_.keys.size(); ++key) {
        if (!applied_.keys[key] || target_.keys[key]) continue;
        if (!output_.key(key, false)) return;
        applied_.keys.reset(key);
        if (repeat_key_ == key) repeat_key_ = 0;
    }
    for (auto &key : applied_.consumer) {
        if (key && !contains(target_.consumer, key)) {
            if (!output_.consumer(key, false)) return;
            key = 0;
        }
    }
    const auto press_key = [&](size_t key) {
        if (!target_.keys[key] || applied_.keys[key]) return true;
        if (output_.key(key, true)) {
            applied_.keys.set(key);
            // HID slots don't encode press order. Choose a deterministic ordinary key.
            if (!(key >= 0xE0 && key <= 0xE7) && keyboard_owners_[key] && output_.repeatable(key)) {
                repeat_key_ = key;
                repeat_at_ = now_ms + typematic.delay_ms;
            }
            return true;
        }
        return false;
    };
    // Modifier makes must precede the ordinary keys in the same snapshot.
    for (size_t key = 0xE0; key <= 0xE7; ++key) if (!press_key(key)) return;
    for (size_t key = 1; key < target_.keys.size(); ++key)
        if ((key < 0xE0 || key > 0xE7) && !press_key(key)) return;
    for (auto key : target_.consumer) {
        if (!key || contains(applied_.consumer, key)) continue;
        auto slot = std::find(applied_.consumer.begin(), applied_.consumer.end(), 0);
        if (slot == applied_.consumer.end() || !output_.consumer(key, true)) return;
        *slot = key;
    }
    const uint8_t config = typematic.config;
    if (repeat_config_ != config) {
        repeat_config_ = config;
        repeat_at_ = now_ms + typematic.delay_ms;
    }
    if (repeat_key_ && applied_.keys[repeat_key_] && target_.keys[repeat_key_] && now_ms >= repeat_at_ && output_.repeatable(repeat_key_)) {
        if (output_.key(repeat_key_, true)) repeat_at_ = now_ms + typematic.cycle_ms;
    }
}
