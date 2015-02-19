#include "memory.hpp"
#include "debug.hpp"
#include <algorithm>
#include <iomanip>

uint8_t gb::memory_map::read8(uint16_t addr) const
{
	if (_dma_mode && !(0xFF80 <= addr && addr <= 0xFFFE))
	{
		debug("WARNING: memory read to non-high-ram while DMA transfer");
	}

	for (const auto &m : _mappings)
	{
		uint8_t value;
		if (m->read8(addr, value))
		{
			return value;
		}
	}

	debug("WARNING: non-mapped read ", addr);
	return 0;
}

void gb::memory_map::write8(uint16_t addr, uint8_t value)
{
	if (_dma_mode && !(0xFF80 <= addr && addr <= 0xFFFE))
	{
		debug("WARNING: memory write to non-high-ram while DMA transfer ignored");
		// TODO return;
	}

	for (const auto &m : _mappings)
	{
		if (m->write8(addr, value))
		{
			return;
		}
	}

	debug("WARNING: non-mapped write ", addr, ": ", static_cast<int>(value));
}

uint16_t gb::memory_map::read16(uint16_t addr) const
{
	uint16_t low = read8(addr);
	uint16_t high = read8(addr + 1);
	return (high << 8) | low;
}

void gb::memory_map::write16(uint16_t addr, uint16_t value)
{
	uint8_t high = value >> 8;
	uint8_t low = value & 0xFF;
	write8(addr, low);
	write8(addr + 1, high);
}
