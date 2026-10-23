#pragma once
#include <cstdint>

struct Typematic {
    uint8_t config;
    uint16_t delay_ms, cycle_ms;
};
// Boundary between input state processing and the hardware protocol.
class InputOutput {
public:
    virtual ~InputOutput() = default;
    virtual uint32_t revision() const { return 0; }
    virtual uint8_t canonical_key(uint8_t code) const { return code; }
    virtual uint8_t consumer_key(uint16_t) const { return 0; }
    virtual bool key(uint8_t code, bool down) = 0;
    virtual bool consumer(uint16_t code, bool down) = 0;
    virtual bool repeatable(uint8_t code) const = 0;
    virtual Typematic typematic() const = 0;
    virtual void mouse(int64_t x, int64_t y, int64_t wheel, uint8_t buttons) = 0;
    // Keep host configuration/reset and one bridge update in distinct transactions.
    class KeyUpdate {
    public:
        explicit KeyUpdate(InputOutput &output) : output_(output) { output_.begin_key_update(); }
        ~KeyUpdate() { output_.end_key_update(); }
        KeyUpdate(const KeyUpdate &) = delete;
        KeyUpdate &operator=(const KeyUpdate &) = delete;
    private:
        InputOutput &output_;
    };
protected:
    virtual void begin_key_update() {}
    virtual void end_key_update() {}
};
