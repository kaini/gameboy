#pragma once
#include "memory.hpp"

namespace gb
{

enum class key
{
	right = 0,  // bit 0
	left,       // bit 1
	up,         // bit 2
	down,       // bit 3
	a,          // bit 0
	b,          // bit 1
	select,     // bit 2
	start,      // bit 3
};

class joypad : public memory_mapping
{
public:
	static const uint8_t direction_keys_bit = 1 << 4;
	static const uint8_t button_keys_bit = 1 << 5;

	joypad();

	bool read8(uint16_t addr, uint8_t & value) const override;
	bool write8(uint16_t addr, uint8_t value) override;

	void down(key key);
	void up(key key);

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
	bool _arrows_select, _buttons_select;
	unsigned int _arrows, _buttons;
};

}
