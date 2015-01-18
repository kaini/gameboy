#pragma once
#include "memory.hpp"
#include <array>

namespace gb
{

class internal_ram : public memory_mapping
{
public:
	static const uint16_t svbk = 0xFF70;
	static const uint16_t if_ = 0xFF0F;
	static const uint16_t ie = 0xFFFF;  // in high ram

	internal_ram();

	bool read8(uint16_t addr, uint8_t &value) const override;
	bool write8(uint16_t addr, uint8_t value) override;

private:
	std::array<uint8_t, 0x8000> _ram;
	std::array<uint8_t, 0x80> _high_ram;
	uint16_t _bank;
	uint8_t _svbk, _if;
};

}
