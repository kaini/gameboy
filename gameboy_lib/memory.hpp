#pragma once
#include "rom.hpp"
#include <cstdint>
#include <vector>

namespace gb
{

class memory_mapping
{
public:
	virtual ~memory_mapping() = default;

	virtual bool read8(uint16_t addr, uint8_t &value) const = 0;
	virtual bool write8(uint16_t addr, uint8_t value) = 0;
};

class memory
{
public:
	memory() : _dma_mode(false) {}

	void add_mapping(memory_mapping *m) { _mappings.emplace_back(m); }

	uint8_t read8(uint16_t addr) const;
	void write8(uint16_t addr, uint8_t value);

	uint16_t read16(uint16_t addr) const;
	void write16(uint16_t addr, uint16_t value);

	void set_dma_mode(bool dma) { _dma_mode = dma; }

private:
	std::vector<memory_mapping *> _mappings;
	bool _dma_mode;
};

}

