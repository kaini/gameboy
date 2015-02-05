#include "cart_mbc5.hpp"
#include "debug.hpp"
#include "bits.hpp"

gb::cart_mbc5::cart_mbc5(rom rom) :
	_ram_enabled(false), _rom_bank(0), _ram_bank(0), _rom(std::move(rom))
{
	std::fill(_ram.begin(), _ram.end(), 0);
}

bool gb::cart_mbc5::read8(uint16_t addr, uint8_t &value) const
{
	if (addr < 0x4000)
	{
		value = _rom.data()[addr];
		return true;
	}
	else if (addr < 0x8000)
	{
		auto real_addr = (addr - 0x4000) + (_rom_bank * 0x4000);
		if (real_addr < _rom.data().size())
		{
			value = _rom.data()[real_addr];
		}
		else
		{
			debug("WARNING: Read after end of ROM: ", real_addr);
			value = 0;
		}
		return true;
	}
	else if (0xA000 <= addr && addr < 0xC000)
	{
		if (!_ram_enabled)
		{
			debug("WARNING: RAM read while not enabled: ", addr);
		}
		auto real_addr = (addr - 0xA000) + (_ram_bank * 0x2000);
		value = _ram[real_addr];
		return true;
	}
	else
	{
		return false;
	}
}

bool gb::cart_mbc5::write8(uint16_t addr, uint8_t value)
{
	if (addr < 0x2000)
	{
		_ram_enabled = bit::test(value, enable_ram_mask);
		return true;
	}
	else if (addr < 0x3000)
	{
		_rom_bank = (_rom_bank & ~0xFF) | value;
		return true;
	}
	else if (addr < 0x4000)
	{
		_rom_bank = (_rom_bank & 0xFF) | ((value & 0x01) << 8);
		return true;
	}
	else if (addr < 0x6000)
	{
		_ram_bank = value & 0x0F;
		return true;
	}
	else if (0xA000 <= addr && addr < 0xC000)
	{
		if (_ram_enabled)
		{
			auto real_addr = (addr - 0xA000) + (_ram_bank * 0x2000);
			_ram[real_addr] = value;
		}
		else
		{
			debug("WARNING: RAM write while not enabled: ", addr);
		}
		return true;
	}
	else
	{
		return false;
	}
}
