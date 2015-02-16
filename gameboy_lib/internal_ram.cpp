#include "internal_ram.hpp"

gb::internal_ram::internal_ram() :
	_bank(1),
	_svbk(0),
	_if(0)
{
	std::fill(_ram.begin(), _ram.end(), 0);
	std::fill(_high_ram.begin(), _high_ram.end(), 0);
}

bool gb::internal_ram::read8(uint16_t addr, uint8_t &value) const
{
	if (0xC000 <= addr && addr < 0xD000)
	{
		value = _ram[addr - 0xC000];
		return true;
	}
	else if (0xD000 <= addr && addr < 0xE000)
	{
		value = _ram[addr - 0xD000 + _bank * 0x1000];
		return true;
	}
	else if (0xE000 <= addr && addr < 0xFE00)
	{
		value = _ram[addr - 0xE000];
		return true;
	}
	else if (0xFF80 <= addr)
	{
		value = _high_ram[addr - 0xFF80];
		return true;
	}
	else switch (addr)
	{
	case svbk:
		value = _svbk;
		return true;
	case if_:
		value = _if;
		return true;
	default:
		return false;
	}
}

bool gb::internal_ram::write8(uint16_t addr, uint8_t value)
{
	if (0xC000 <= addr && addr < 0xD000)
	{
		_ram[addr - 0xC000] = value;
		return true;
	}
	else if (0xD000 <= addr && addr < 0xE000)
	{
		_ram[addr - 0xD000 + _bank * 0x1000] = value;
		return true;
	}
	else if (0xE000 <= addr && addr < 0xFE00)
	{
		_ram[addr - 0xE000] = value;
		return true;
	}
	else if (0xFF80 <= addr)
	{
		_high_ram[addr - 0xFF80] = value;
		return true;
	}
	else switch (addr)
	{
	case svbk:
		_bank = value & 0x07;
		if (_bank == 0)
			_bank = 1;
		_svbk = value;
		return true;
	case if_:
		_if = value;
		return true;
	default:
		return false;
	}
}
