#include "cart_rom_only.hpp"

gb::cart_rom_only::cart_rom_only(rom rom) :
	_rom(std::move(rom))
{
	std::fill(_ram.begin(), _ram.end(), 0);
}

bool gb::cart_rom_only::read8(uint16_t addr, uint8_t &value) const
{
	if (addr < 0x8000)
	{
		value = _rom.data()[addr];
		return true;
	}
	else if (0xA000 <= addr && addr < 0xC000)
	{
		value = _ram[addr - 0xA000];
		return true;
	}
	else
	{
		return false;
	}
}

bool gb::cart_rom_only::write8(uint16_t addr, uint8_t value)
{
	if (0xA000 <= addr && addr < 0xC000)
	{
		_ram[addr - 0xA000] = value;
		return true;
	}
	else
	{
		return false;
	}
}
