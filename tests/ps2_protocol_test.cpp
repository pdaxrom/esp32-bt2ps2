#include "esp32-ps2dev.h"
#include "ps2_output.hpp"
#include "ps2_timing.hpp"
#include "input_bridge.hpp"
#include "ps2_test_bus.hpp"
#include <cassert>
#include <cstdio>
#include <limits>

using esp32_ps2dev::PS2Keyboard;
using esp32_ps2dev::PS2Mouse;
using esp32_ps2dev::PS2Packet;
namespace {
uint8_t leds = 0;
void led_callback(uint8_t value) { leds = value; }
template<class Device> void command(Device &device, uint8_t byte)
{
    test_bus::request(byte);
    device.service_bus();
}
void expect(std::initializer_list<uint8_t> values, int line)
{
    if (test_bus::bytes() != std::vector<uint8_t>(values)) {
        fprintf(stderr, "Line %d expected:", line);
        for (auto byte : values) fprintf(stderr, " %02X", byte);
        fprintf(stderr, " received:");
        for (auto byte : test_bus::bytes()) fprintf(stderr, " %02X", byte);
        fprintf(stderr, "\n");
    }
    assert(test_bus::bytes() == std::vector<uint8_t>(values));
    test_bus::clear();
}
#define expect(...) expect(__VA_ARGS__, __LINE__)
void keyboard_commands()
{
    test_bus::select(22, 23);
    PS2Keyboard keyboard(22, 23); keyboard.set_leds_callback(led_callback); keyboard.begin(); expect({0xAA});
    assert(keyboard.typematic().delay_ms == 500 && keyboard.typematic().cycle_ms == 92);
    command(keyboard, 0xF0); expect({0xFA});
    vTaskDelay(100); command(keyboard, 3); expect({0xFA});
    command(keyboard, 0xF0); command(keyboard, 0); expect({0xFA,0xFA,3});
    command(keyboard, 0xF0); command(keyboard, 1); expect({0xFA,0xFE});
    command(keyboard, 2); expect({0xFA});
    command(keyboard, 0xED); vTaskDelay(100); command(keyboard, 6); expect({0xFA,0xFA});
    assert(leds == 6 && keyboard.is_caps_lock_led_on() && keyboard.is_num_lock_led_on());
    command(keyboard, 0xF3); expect({0xFA});
    assert(keyboard.keyHid_send(4, true));
    keyboard.service_bus(); expect({}); // No scan codes while waiting for the argument.
    vTaskDelay(100); command(keyboard, 0x20); expect({0xFA,0x1C});
    assert(keyboard.typematic().delay_ms == 500 && keyboard.typematic().cycle_ms == 33);
    command(keyboard, 0xF5); expect({0xFA}); assert(!keyboard.keyHid_send(4, true));
    command(keyboard, 0xF4); expect({0xFA}); assert(keyboard.keyHid_send(4, true));
    keyboard.service_bus(); expect({0x1C});
    // Previously this waited forever under both bus and state mutexes.
    test_bus::inhibit_after(1);
    command(keyboard, 0xF2);
    assert(test_bus::bytes() == std::vector<uint8_t>({0xFA}));
    assert(keyboard.keyHid_send(5, true)); // State mutex is available during the pause.
    test_bus::inhibit(false); keyboard.service_bus(); expect({0xFA,0xAB,0x83,0x32});
    command(keyboard, 0xFE); expect({0x32});
    // A Reset interrupts an unfinished reply and cancels the previously queued scan codes.
    assert(keyboard.keyHid_send(6, true));
    test_bus::request_after(1, 0xFF); keyboard.reply_to_host(0xF2);
    keyboard.service_bus(); expect({0xFA,0xFA,0xAA});
    assert(leds == 0);
    command(keyboard, 0xEF); expect({0xFE});
    command(keyboard, 0xFE); expect({0xAA}); // Error response doesn't replace RESEND history.
    command(keyboard, 0xF3); command(keyboard, 0x7F); expect({0xFA,0xFA});
    assert(keyboard.typematic().delay_ms == 1000 && keyboard.typematic().cycle_ms == 500);
    // Bad wire parity/stop bits request retransmission instead of executing the command.
    test_bus::request(0xF5, true); keyboard.service_bus(); expect({0xFE});
    assert(keyboard.data_reporting_enabled());
    test_bus::request(0xF5, false, true); keyboard.service_bus(); expect({0xFE});
    assert(keyboard.data_reporting_enabled());
}
void mouse_commands()
{
    test_bus::select(26, 25);
    PS2Mouse mouse(26, 25); mouse.begin(); expect({0xAA,0});
    command(mouse, 0xFF); expect({0xFA,0xAA,0});
    command(mouse, 0xFE); expect({0xAA,0});
    command(mouse, 0xD0); expect({0xFE});
    command(mouse, 0xFE); expect({0xAA,0});
    const auto rate = [&](uint8_t value) { command(mouse, 0xF3); vTaskDelay(100); command(mouse, value); expect({0xFA,0xFA}); };
    rate(200); rate(100); rate(80); command(mouse, 0xF2); expect({0xFA,3}); assert(mouse.has_wheel());
    rate(100); command(mouse, 0xF2); expect({0xFA,3}); // Negotiated ID survives an ordinary rate change.
    rate(200); rate(200); rate(80); command(mouse, 0xF2); expect({0xFA,4});
    command(mouse, 0xFF); expect({0xFA,0xAA,0}); command(mouse, 0xF2); expect({0xFA,0});
    command(mouse, 0xF4); expect({0xFA}); mouse._report(); mouse.service_bus(); expect({8,0,0});
    command(mouse, 0xF0); expect({0xFA});
    mouse.apply_input(2, -3, 0, 1); mouse._report(); mouse.service_bus(); expect({});
    command(mouse, 0xEB); expect({0xFA,9,2,3}); command(mouse, 0xFE); expect({9,2,3});
    command(mouse, 0xEE); expect({0xFA}); assert(!mouse.stream_reporting_enabled());
    command(mouse, 0xE9); expect({0xE9}); command(mouse, 0xEC); expect({0xFA});
    assert(!mouse.stream_reporting_enabled()); // Restores remote mode.
    command(mouse, 0xEA); expect({0xFA}); assert(mouse.stream_reporting_enabled());
    // Queuing failure retains movement and the button state until capacity returns.
    PS2Packet filler{}; filler.len = 1; filler.data[0] = 0x77;
    for (int i = 0; i < esp32_ps2dev::PACKET_QUEUE_LENGTH; ++i) assert(mouse.send_packet(&filler) == 0);
    mouse.apply_input(7, -8, 0, 2); mouse._report();
    xQueueReset(mouse.get_packet_queue_handle()); mouse._report(); mouse.service_bus(); expect({10,7,8});
    // Press and release between sampler ticks must still produce two button states.
    mouse.apply_input(0, 0, 0, 3); mouse.apply_input(0, 0, 0, 2);
    mouse._report(); mouse.service_bus(); expect({11,0,0});
    mouse._report(); mouse.service_bus(); expect({10,0,0});
    // Extreme input is clamped before negation (UBSan exercises INT64_MIN).
    command(mouse, 0xF0); expect({0xFA});
    mouse.move(0, std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::min());
    command(mouse, 0xEB); expect({0xFA,0x8A,0,255});
}
void bridge_after_reset()
{
    test_bus::select(22, 23);
    PS2Keyboard keyboard(22, 23); keyboard.begin(); expect({0xAA});
    PS2Mouse mouse(26, 25); mouse.PS2dev::begin(1);
    Ps2Output output(keyboard, mouse); InputBridge bridge(output);
    input::Snapshot held; held.keys.set(4); held.keys.set(0xE1);
    bridge.accept(held); bridge.tick(0); keyboard.service_bus(); keyboard.service_bus(); expect({0x12,0x1C});
    command(keyboard, 0xFF); expect({0xFA,0xAA});
    bridge.tick(1); keyboard.service_bus(); keyboard.service_bus(); expect({0x12,0x1C});
    input::Snapshot empty; bridge.accept(empty); bridge.tick(2);
    keyboard.service_bus(); keyboard.service_bus(); expect({0xF0,0x1C,0xF0,0x12});
    input::Snapshot aliases; aliases.keys.set(0x7F); aliases.consumer[0] = 0xE2;
    bridge.accept(aliases); bridge.tick(3); keyboard.service_bus(); expect({0xE0,0x23});
    aliases.keys.reset(0x7F); bridge.accept(aliases); bridge.tick(4); keyboard.service_bus(); expect({});
    bridge.accept(empty); bridge.tick(5); keyboard.service_bus(); expect({0xE0,0xF0,0x23});
    input::Snapshot backslash; backslash.keys.set(0x31); backslash.keys.set(0x32);
    bridge.accept(backslash); bridge.tick(6); keyboard.service_bus(); expect({0x5D});
    backslash.keys.reset(0x31); bridge.accept(backslash); bridge.tick(7); keyboard.service_bus(); expect({});
    bridge.accept(empty); bridge.tick(8); keyboard.service_bus(); expect({0xF0,0x5D});
    input::Snapshot iso; iso.keys.set(0x64);
    bridge.accept(iso); bridge.tick(9); keyboard.service_bus(); expect({0x61});
    bridge.accept(empty); bridge.tick(10); keyboard.service_bus(); expect({0xF0,0x61});
    input::Snapshot media; media.consumer[0] = 0xCD;
    bridge.accept(media); bridge.tick(11); keyboard.service_bus(); expect({0xE0,0x34});
    bridge.tick(1000); keyboard.service_bus(); expect({}); // Consumer-only ownership must not start typematic.
    bridge.accept(empty); bridge.tick(1001); keyboard.service_bus(); expect({0xE0,0xF0,0x34});
}
void stop_completion_backpressure()
{
    test_bus::select(22, 23);
    PS2Keyboard keyboard(22, 23); keyboard.begin(); expect({0xAA});
    test_bus::inhibit_stop_after(1);
    assert(keyboard.write(0x5A) == 0);
    assert(test_bus::stop_received() == std::vector<uint8_t>({0x5A}));
    test_bus::inhibit(false); expect({});
    // The break's second byte has already reached the host: resume must
    // advance the packet rather than resend it as a new make code.
    assert(keyboard.keyHid_send(0x28, false));
    test_bus::inhibit_stop_after(2); keyboard.service_bus();
    assert(test_bus::stop_received() == std::vector<uint8_t>({0xF0,0x5A}));
    assert(test_bus::bytes() == std::vector<uint8_t>({0xF0}));
    test_bus::inhibit(false); keyboard.service_bus(); expect({0xF0});
}
void wire_inhibition()
{
    test_bus::select(22, 23);
    PS2Keyboard keyboard(22, 23); keyboard.begin(); expect({0xAA});
    for (unsigned bit = 1; bit <= 10; ++bit) {
        test_bus::inhibit_at_bit(bit);
        assert(keyboard.write(0x1C) != 0);
        assert(test_bus::bytes().empty());
        test_bus::inhibit(false); assert(keyboard.write(0x1C) == 0); expect({0x1C});
    }
    // A paused multi-byte key break must keep its position across worker iterations.
    assert(keyboard.keyHid_send(4, false));
    test_bus::inhibit_after(1); keyboard.service_bus();
    assert(test_bus::bytes() == std::vector<uint8_t>({0xF0}));
    test_bus::inhibit(false); keyboard.service_bus(); expect({0xF0,0x1C});
    test_bus::time(int64_t{UINT32_MAX} - 20);
    esp32_ps2dev::delayMicroseconds(40); // Hardware's 32-bit micros rollover.
    test_bus::time(int64_t{UINT32_MAX} - 100);
    test_bus::inhibit(true); assert(keyboard.write_wait_idle(0x1C, 1500) != 0);
    test_bus::inhibit(false); assert(keyboard.write(0x1C) == 0); expect({0x1C});
}
}
int main()
{
    keyboard_commands(); mouse_commands(); bridge_after_reset(); wire_inhibition(); stop_completion_backpressure();
    puts("PS/2 driver integration: commands, delayed arguments, inhibition, RESEND, reset and queue recovery passed");
}
