#pragma once
#include "memory.hpp"

namespace gb
{

// TODO implement this thing
class joypad : public memory_mapping
{
public:
	joypad();

	bool read8(uint16_t addr, uint8_t & value) const override;
	bool write8(uint16_t addr, uint8_t value) override;

private:
	bool _button_keys;
	bool _direction_keys;
};

}
