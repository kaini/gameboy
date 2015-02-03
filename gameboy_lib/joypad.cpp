#include "joypad.hpp"
#include "bits.hpp"

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
			bit::set(value, button_keys_bit);
		if (_direction_keys)
			bit::set(value, direction_keys_bit);
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
		_button_keys = bit::test(value, button_keys_bit);
		_direction_keys = bit::test(value, direction_keys_bit);
		return true;
	}
	else
	{
		return false;
	}
}
