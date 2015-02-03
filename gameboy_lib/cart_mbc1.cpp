#include "cart_mbc1.hpp"
#include "debug.hpp"
#include "bits.hpp"
#include <algorithm>

gb::cart_mbc1::cart_mbc1(rom rom) :
	_rom(rom),
	_ram_enabled(false),
	_rom_bank_low(0),
	_ram_rom_bank(0),
	_ram_mode(false)
{
	std::fill(_ram.begin(), _ram.end(), 0);
}

bool gb::cart_mbc1::read8(uint16_t addr, uint8_t &value) const
{
	if (addr < 0x4000)
	{
		value = _rom.data()[addr];
		return true;
	}
	else if (0x4000 <= addr && addr < 0x8000)
	{
		size_t bank = _rom_bank_low;
		if (bank == 0)
			bank = 1;
		if (!_ram_mode)
			bank |= _ram_rom_bank << 5;
		size_t rom_addr = addr - 0x4000 + bank * 0x4000;
		if (rom_addr < _rom.data().size())
		{
			value = _rom.data()[rom_addr];
		}
		else
		{
			debug("WARNING: invalid read from ROM bank, too high ", addr);
			value = 0;
		}
		return true;
	}
	else if (0xA000 <= addr && addr < 0xC000)
	{
		auto ram_addr = to_ram_addr(addr);
		if (true || ram_addr < _rom.ram_size())  // TODO this check?
		{
			value = _ram[ram_addr];
		}
		else
		{
			debug("WARNING: invalid read in RAM, too high ", addr);
			value = 0;
		}
		return true;
	}
	else
	{
		return false;
	}
}

bool gb::cart_mbc1::write8(uint16_t addr, uint8_t value)
{
	if (addr < 0x2000)
	{
		_ram_enabled = bit::test(value, enable_ram_mask);
		return true;
	}
	else if (addr < 0x4000)
	{
		_rom_bank_low = value & 0x1F;
		return true;
	}
	else if (addr < 0x6000)
	{
		_ram_rom_bank = value & 0x3;
		return true;
	}
	else if (addr < 0x8000)
	{
		_ram_mode = bit::test(value, ram_mode_mask);
		return true;
	}
	else if (0xA000 <= addr && addr < 0xC000)
	{
		if (_ram_enabled)
		{
			auto ram_addr = to_ram_addr(addr);
			if (ram_addr < _rom.ram_size())
			{
				_ram[ram_addr] = value;
			}
			else
			{
				debug("WARNING: invalid write in RAM, too high ", addr);
			}
		}
		return true;
	}
	else
	{
		return false;
	}
}

size_t gb::cart_mbc1::to_ram_addr(uint16_t addr) const
{
	size_t bank = 0;
	if (_ram_mode)
		bank = _ram_rom_bank;
	return addr - 0xA000 + 0x2000 * bank;
}
