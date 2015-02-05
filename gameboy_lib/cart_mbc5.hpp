#pragma once
#include "memory.hpp"
#include "rom.hpp"
#include <array>

namespace gb
{

class cart_mbc5 : public memory_mapping
{
public:
	static const uint8_t enable_ram_mask = 0x0A;

	cart_mbc5(rom rom);

	bool read8(uint16_t addr, uint8_t &value) const override;
	bool write8(uint16_t addr, uint8_t value) override;

private:
	bool _ram_enabled;
	size_t _rom_bank;
	size_t _ram_bank;
	std::array<uint8_t, 0x2000 * 0x10> _ram;
	rom _rom;
};

}
