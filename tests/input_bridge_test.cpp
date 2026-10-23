#include "input_bridge.hpp"
#include <cassert>
#include <cstdio>
#include <tuple>
#include <vector>

struct Output final : InputOutput {
    std::vector<std::tuple<uint16_t, bool, bool>> events;
    bool blocked = false;
    uint16_t fail_once = 0;
    int64_t x = 0, y = 0, wheel = 0;
    uint8_t buttons = 0;
    uint32_t version = 0;
    bool in_update = false;
    void begin_key_update() override { assert(!in_update); in_update = true; }
    void end_key_update() override { assert(in_update); in_update = false; }
    uint32_t revision() const override { assert(in_update); return version; }
    Typematic repeat{0x2B, 500, 100};
    bool key(uint8_t code, bool down) override {
        assert(in_update);
        if (fail_once == code) { fail_once = 0; return false; }
        if (blocked) return false;
        events.emplace_back(code, down, false); return true;
    }
    bool consumer(uint16_t code, bool down) override {
        assert(in_update);
        if (blocked) return false;
        events.emplace_back(code, down, true); return true;
    }
    bool repeatable(uint8_t code) const override { return code < 0xE0; }
    Typematic typematic() const override { return repeat; }
    void mouse(int64_t dx, int64_t dy, int64_t dw, uint8_t b) override { x+=dx; y+=dy; wheel+=dw; buttons=b; }
};
int main()
{
    Output output;
    InputBridge bridge(output);
    input::Snapshot first;
    first.keys.set(4); first.keys.set(0xE1); first.consumer[0]=0xCD;
    first.x=10; first.buttons=3;
    bridge.accept(first); bridge.tick(0);
    assert(output.events.size()==3);
    assert(output.events[0]==std::make_tuple(uint16_t(0xE1),true,false));
    assert(output.events[1]==std::make_tuple(uint16_t(4),true,false));
    assert(output.x==10 && output.buttons==3);
    bridge.tick(499); assert(output.events.size()==3);
    bridge.tick(500); assert(output.events.size()==4);
    bridge.tick(550); assert(output.events.size()==4);
    // A lost snapshot still delivers total relative movement exactly once.
    auto second=first; second.x=25;
    bridge.accept(second); bridge.accept(second);
    assert(output.x==25);
    output.blocked=true;
    input::Snapshot released; released.x=25;
    bridge.accept(released); bridge.tick(600);
    assert(output.events.size()==4);
    output.blocked=false; bridge.tick(605);
    assert(output.events.size()==7); // Both keyboard releases and consumer release retried.
    assert(output.buttons==0 && output.x==25);
    bridge.tick(1000); assert(output.events.size()==7); // Disconnection stops autorepeat.
    // A failed make must also be retried; repeated snapshots don't duplicate it.
    output.blocked=true; bridge.accept(first); bridge.tick(1010);
    assert(output.events.size()==7);
    output.blocked=false; bridge.tick(1020); assert(output.events.size()==10);
    bridge.accept(first); bridge.tick(1021); assert(output.events.size()==10);
    output.repeat={0x20,250,50}; bridge.tick(1100);
    bridge.tick(1349); assert(output.events.size()==10);
    bridge.tick(1350); assert(output.events.size()==11);
    ++output.version; bridge.tick(1400);
    assert(output.events.size()==14); // Host reset re-applies held modifier, key and consumer.
    // A queue slot can become available between calls: no letter may overtake a failed modifier.
    Output partial;
    InputBridge partial_bridge(partial);
    partial.fail_once = 0xE1;
    partial_bridge.accept(first); partial_bridge.tick(0);
    assert(partial.events.empty());
    partial_bridge.tick(1);
    assert(partial.events.size() == 3 && std::get<0>(partial.events[0]) == 0xE1);
    auto next = first; next.keys.reset(0xE1); next.keys.reset(4); next.keys.set(5);
    partial.fail_once = 4;
    partial_bridge.accept(next); partial_bridge.tick(2);
    assert(partial.events.size() == 3); // Failed release must precede every new make.
    partial_bridge.tick(3);
    assert(partial.events.size() == 6);
    Output repeats;
    InputBridge repeat_bridge(repeats);
    input::Snapshot chord; chord.keys.set(4); chord.keys.set(5);
    repeat_bridge.accept(chord); repeat_bridge.tick(0); repeat_bridge.tick(500);
    assert(repeats.events.size() == 3 && std::get<0>(repeats.events.back()) == 5);
    chord.keys.reset(5); repeat_bridge.accept(chord); repeat_bridge.tick(501);
    repeat_bridge.tick(1500);
    assert(repeats.events.size() == 4); // Releasing the typematic key must not restart an older held key.
    puts("Input bridge ordering, autorepeat, disconnect and backpressure tests passed");
}
