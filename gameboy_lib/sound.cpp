#include "sound.hpp"

bool gb::sound::read8(uint16_t addr, uint8_t &value) const
{
	if (0xFF10 <= addr && addr <= 0xFF3F)
	{
		value = _memory[addr - 0xFF10];
		return true;
	}
	else
	{
		return false;
	}
}

bool gb::sound::write8(uint16_t addr, uint8_t value)
{
	if (0xFF10 <= addr && addr <= 0xFF3F)
	{
		_memory[addr - 0xFF10] = value;
		return true;
	}
	else
	{
		return false;
	}
}

