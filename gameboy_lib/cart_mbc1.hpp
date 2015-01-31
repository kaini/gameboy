#pragma once
#include "memory.hpp"
#include "rom.hpp"
#include <array>
#include <cstdint>

namespace gb
{

class cart_mbc1 final : public memory_mapping
{
public:
	cart_mbc1(rom rom);

	bool read8(uint16_t addr, uint8_t &value) const override;
	bool write8(uint16_t addr, uint8_t value) override;

private:
	size_t to_ram_addr(uint16_t addr) const;

	const rom _rom;
	bool _ram_enabled; 
	uint8_t _rom_bank_low;
	uint8_t _ram_rom_bank;
	bool _ram_mode;
	std::array<uint8_t, 0x8000> _ram;
};

}
