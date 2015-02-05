#pragma once
#include "memory.hpp"
#include <array>

namespace gb
{

// TODO implement sound
class sound : public memory_mapping
{
public:
	bool read8(uint16_t addr, uint8_t &value) const override;
	bool write8(uint16_t addr, uint8_t value) override;

private:
	std::array<uint8_t, 0x30> _memory;
};

}