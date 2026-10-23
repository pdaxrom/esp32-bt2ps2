#pragma once
#include "input_state.hpp"
#include "input_output.hpp"

// Owned by the application input task; no Bluetooth or descriptor parsing here.
class InputBridge {
public:
    explicit InputBridge(InputOutput &output) : output_(output) {}
    void accept(const input::Snapshot &snapshot);
    void tick(int64_t now_ms);
private:
    InputOutput &output_;
    input::Snapshot target_, applied_;
    std::bitset<256> keyboard_owners_;
    uint8_t repeat_key_ = 0, repeat_config_ = 0;
    int64_t repeat_at_ = 0;
    uint32_t revision_ = 0;
};
