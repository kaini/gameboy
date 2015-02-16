#pragma once
#include "memory.hpp"
#include <chrono>

namespace gb
{

class z80_cpu;

class timer : public memory_mapping
{
public:
	/** Divider */
	static const uint16_t div = 0xFF04;
	/** Time counter */
	static const uint16_t tima = 0xFF05;
	/** Timer modulo */
	static const uint16_t tma = 0xFF06;
	/** Timer control */
	static const uint16_t tac = 0xFF07;

	static const std::chrono::nanoseconds tick_ns;    // 16384 Hz
	static const std::chrono::nanoseconds tick_fast_ns;  // 32768 Hz
	static const std::chrono::nanoseconds tima_0_ns;  // 4096 Hz
	static const std::chrono::nanoseconds tima_1_ns;  // 262144 Hz
	static const std::chrono::nanoseconds tima_2_ns;  // 65536 Hz
	static const std::chrono::nanoseconds tima_3_ns;  // 16384 Hz

	timer();

	bool read8(uint16_t addr, uint8_t & value) const override;
	bool write8(uint16_t addr, uint8_t value) override;
	void tick(z80_cpu &cpu, std::chrono::nanoseconds ns);

private:
	uint8_t _div, _tima, _tma, _tac;
	std::chrono::nanoseconds _last_div_increment, _last_tima_increment;
};

}
