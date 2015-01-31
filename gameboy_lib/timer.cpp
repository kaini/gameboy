#include "timer.hpp"
#include "z80.hpp"

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

void gb::timer::tick(z80_cpu &cpu, int ns)
{
	int div_increment_at = tick_ns;
	if (cpu.double_speed())
		div_increment_at /= 2;

	_last_div_increment += ns;
	if (_last_div_increment >= div_increment_at)
	{
		++_div;
		_last_div_increment = 0;
	}

	if (_tac & 0x04)
	{
		int tima_increment_at;
		switch (_tac & 0x03)
		{
		case 0:
			tima_increment_at = 244140;  // 4096 Hz
			break;
		case 1:
			tima_increment_at = 3726;  // 262144 Hz
			break;
		case 2:
			tima_increment_at = 15259;  // 65536 Hz
			break;
		case 3:
			tima_increment_at = 61035;  // 16384 Hz
			break;
		}

		_last_tima_increment += ns;
		if (_last_tima_increment >= tima_increment_at)
		{
			++_tima;
			_last_tima_increment = 0;
			if (_tima == 0)
			{
				_tima = _tma;
				cpu.post_interrupt(interrupt::timer);
			}
		}
	}
}
