#include "timer.hpp"
#include "z80.hpp"
#include "assert.hpp"

const gb::cputime gb::timer::tick_time(512);     // 1 / 2^14 == 1 / 2^23 * 2^9
const gb::cputime gb::timer::tima_0_time(2048);  // 1 / 2^12 == 1 / 2^23 * 2^11
const gb::cputime gb::timer::tima_1_time(32);    // 1 / 2^18 == 1 / 2^23 * 2^5
const gb::cputime gb::timer::tima_2_time(128);   // 1 / 2^16 == 1 / 2^23 * 2^7
const gb::cputime gb::timer::tima_3_time(512);   // 1 / 2^14 == 1 / 2^23 * 2^9

gb::timer::timer() :
	_div(0), _tima(0), _tma(0), _tac(0), _last_div_increment(0), _last_tima_increment(0)
{
}

bool gb::timer::read8(uint16_t addr, uint8_t &value) const
{
	switch (addr)
	{
	case div:
		value = _div;
		return true;
	case tima:
		value = _tima;
		return true;
	case tma:
		value = _tma;
		return true;
	case tac:
		value = _tac;
		return true;
	default:
		return false;
	}
}

bool gb::timer::write8(uint16_t addr, uint8_t value)
{
	switch (addr)
	{
	case div:
		_div = 0;  // resets on write
		return true;
	case tima:
		_tima = value;
		return true;
	case tma:
		_tma = value;
		return true;
	case tac:
		_tac = value;
		return true;
	default:
		return false;
	}
}

void gb::timer::tick(z80_cpu &cpu, cputime time)
{
	using namespace std::chrono;

	auto div_increment_at = tick_time;
	if (cpu.double_speed())
		div_increment_at /= 2;
	_last_div_increment += time;
	while (_last_div_increment >= div_increment_at)
	{
		++_div;
		_last_div_increment -= div_increment_at;
	}

	if (_tac & 0x04)
	{
		cputime tima_increment_at;
		switch (_tac & 0x03)
		{
		case 0:
			tima_increment_at = tima_0_time;  // 4096 Hz
			break;
		case 1:
			tima_increment_at = tima_1_time;  // 262144 Hz
			break;
		case 2:
			tima_increment_at = tima_2_time;  // 65536 Hz
			break;
		case 3:
			tima_increment_at = tima_3_time;  // 16384 Hz
			break;
		default:
			ASSERT_UNREACHABLE();
		}
		if (cpu.double_speed())
			tima_increment_at /= 2;

		_last_tima_increment += time;
		while (_last_tima_increment >= tima_increment_at)
		{
			++_tima;
			_last_tima_increment -= tima_increment_at;
			if (_tima == 0)
			{
				_tima = _tma;
				cpu.post_interrupt(interrupt::timer);
			}
		}
	}
}
