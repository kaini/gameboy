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
	bool _button_keys;
	bool _direction_keys;
};

}
