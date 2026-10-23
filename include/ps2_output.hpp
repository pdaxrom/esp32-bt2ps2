#pragma once
#include "input_output.hpp"
#include "esp32-ps2dev.h"

class Ps2Output final : public InputOutput {
public:
    Ps2Output(esp32_ps2dev::PS2Keyboard &keyboard, esp32_ps2dev::PS2Mouse &mouse)
        : keyboard_(keyboard), mouse_(mouse) {}
    uint32_t revision() const override { return keyboard_.packet_generation(); }
    uint8_t canonical_key(uint8_t code) const override { return keyboard_.canonical_hid_key(code); }
    uint8_t consumer_key(uint16_t code) const override { return keyboard_.consumer_hid_key(code); }
    bool key(uint8_t code, bool down) override { return keyboard_.keyHid_send(code, down); }
    bool consumer(uint16_t code, bool down) override { return keyboard_.keyHid_send_CCONTROL(code, down); }
    bool repeatable(uint8_t code) const override { return keyboard_.allows_typematic(code); }
    Typematic typematic() const override {
        return keyboard_.typematic();
    }
    void mouse(int64_t x, int64_t y, int64_t wheel, uint8_t buttons) override {
        mouse_.apply_input(x, y, wheel, buttons);
    }
private:
    void begin_key_update() override { keyboard_.begin_input_update(); }
    void end_key_update() override { keyboard_.end_input_update(); }
    esp32_ps2dev::PS2Keyboard &keyboard_;
    esp32_ps2dev::PS2Mouse &mouse_;
};
