#pragma once
#include <cstdint>
#include <vector>
namespace test_bus {
void select(int clock, int data);
void inhibit(bool active);
void inhibit_after(unsigned complete_bytes);
void inhibit_at_bit(unsigned bit);
void inhibit_stop_after(unsigned bytes);
const std::vector<uint8_t> &stop_received();
void request(uint8_t byte, bool bad_parity = false, bool bad_stop = false);
void request_after(unsigned complete_bytes, uint8_t byte);
void clear();
void time(int64_t microseconds);
const std::vector<uint8_t> &bytes();
}
