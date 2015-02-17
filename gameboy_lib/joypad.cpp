#include "joypad.hpp"
#include "bits.hpp"

gb::joypad::joypad() :
	_register(0x3F /* 0011 1111 */)
{
}

bool gb::joypad::read8(uint16_t addr, uint8_t &value) const
{
	if (addr == 0xFF00)
	{
		value = _register;
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
		_register &= ~0x30;
		_register |= value & 0x30;
		return true;
	}
	else
	{
		return false;
	}
}
