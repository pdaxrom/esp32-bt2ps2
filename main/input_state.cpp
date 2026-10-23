#include "input_state.hpp"

namespace input {
bool State::aggregate(Snapshot &out) const
{
    out.keys.reset(); out.consumer.fill(0); out.buttons = 0;
    for (const auto &entry : reports_) {
        out.keys |= entry.second.keys;
        out.buttons |= entry.second.buttons;
        for (auto code : entry.second.consumer) {
            if (!code) continue;
            bool added = false;
            for (auto &slot : out.consumer) {
                if (slot == code) { added = true; break; }
                if (!slot) { slot = code; added = true; break; }
            }
            if (!added) return false;
        }
    }
    return true;
}
bool State::update(Source source, const hid::Input &report)
{
    auto found = reports_.find(source);
    const bool existed = found != reports_.end();
    hid::Input previous;
    if (existed) previous = found->second;
    reports_[source] = report;
    Snapshot next = snapshot_;
    if (!aggregate(next)) {
        if (existed) reports_[source] = previous;
        else reports_.erase(source);
        return false;
    }
    next.x += report.x; next.y += report.y; next.wheel += report.wheel;
    snapshot_ = next;
    return true;
}
void State::disconnect(uintptr_t device)
{
    for (auto it = reports_.begin(); it != reports_.end();) {
        if (it->first.device == device) it = reports_.erase(it);
        else ++it;
    }
    aggregate(snapshot_);
}
} // namespace input
