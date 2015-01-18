#pragma once
#include "memory.hpp"
#include "rom.hpp"
#include <array>

namespace gb
{

class cart_rom_only final : public memory_mapping
{
public:
	cart_rom_only(rom rom);

	bool read8(uint16_t addr, uint8_t &value) const override;
	bool write8(uint16_t addr, uint8_t value) override;

private:
	const rom _rom;
	std::array<uint8_t, 0x2000> _ram;
};

}
