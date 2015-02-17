#pragma once
#include "memory.hpp"

namespace gb
{

// TODO implement this thing
class joypad : public memory_mapping
{
public:
	static const uint8_t direction_keys_bit = 1 << 4;
	static const uint8_t button_keys_bit = 1 << 5;

	joypad();

	bool read8(uint16_t addr, uint8_t & value) const override;
	bool write8(uint16_t addr, uint8_t value) override;

private:
	// Bit 7  -
	//     6  -
	//     5  Button Keys Select (0 = select)
	//     4  Direction Keys Select (0 = select)
	//     3  Down/Start
	//     2  Up/Select
	//     1  Left/B
	//     0  Right/A
	// IMPORTANT: 1 = released and 0 = pressed
	uint8_t _register;
};

}
