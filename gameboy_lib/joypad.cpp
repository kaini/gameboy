#include "joypad.hpp"

gb::joypad::joypad() :
	_button_keys(false), _direction_keys(false)
{
}

bool gb::joypad::read8(uint16_t addr, uint8_t &value) const
{
	if (addr == 0xFF00)
	{
		value = 0;
		if (_button_keys)
			value |= 1 << 5;
		if (_direction_keys)
			value |= 1 << 4;
		return true;
	}
	else
	{
		return false;
	}
}

bool gb::joypad::write8(uint16_t addr, uint8_t value)
{
	if (addr == 0xFF00)
	{
		_button_keys = (value & (1 << 5)) != 0;
		_direction_keys = (value & (1 << 4)) != 0;
		return true;
	}
	else
	{
		return false;
	}
}
