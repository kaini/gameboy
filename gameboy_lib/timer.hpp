#pragma once
#include "memory.hpp"
#include "time.hpp"

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

	static const cputime tick_time;       // 16384 Hz
	static const cputime tima_0_time;     // 4096 Hz
	static const cputime tima_1_time;     // 262144 Hz
	static const cputime tima_2_time;     // 65536 Hz
	static const cputime tima_3_time;     // 16384 Hz

	timer();

	bool read8(uint16_t addr, uint8_t & value) const override;
	bool write8(uint16_t addr, uint8_t value) override;
	void tick(z80_cpu &cpu, cputime time);

private:
	uint8_t _div, _tima, _tma, _tac;
	cputime _last_div_increment, _last_tima_increment;
};

}
