#include "z80.hpp"
#include "z80opcodes.hpp"
#include "internal_ram.hpp"
#include "debug.hpp"
#include "assert.hpp"
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>

const gb::cputime gb::z80_cpu::clock(2);       // 1 / 2^22
const gb::cputime gb::z80_cpu::clock_fast(1);  // 1 / 2^23 == clock / 2

std::string gb::to_string(register8 r)
{
	switch (r)
	{
	case register8::a:
		return "A";
	case register8::f:
		return "F";
	case register8::b:
		return "B";
	case register8::c:
		return "C";
	case register8::d:
		return "D";
	case register8::e:
		return "E";
	case register8::h:
		return "H";
	case register8::l:
		return "L";
	default:
		ASSERT_UNREACHABLE();
	}
}

std::string gb::to_string(register16 r)
{
	switch (r)
	{
	case register16::af:
		return "AF";
	case register16::bc:
		return "BC";
	case register16::de:
		return "DE";
	case register16::hl:
		return "HL";
	case register16::sp:
		return "SP";
	case register16::pc:
		return "PC";
	default:
		ASSERT_UNREACHABLE();
	}
}

gb::register_file::register_file()
{
	_pc = _sp = _a = _c = _b = _e = _d = _l = _h = 0;
	_flag_z = _flag_n = _flag_h = _flag_c = false;
}

void gb::register_file::debug_print() const
{
	char buffer[200];
	sprintf(buffer,
		"AF=%02x%02x  BC=%02x%02x  DE=%02x%02x  HL=%02x%02x  SP=%04x  PC=%04x  [%c%c%c%c]",
		static_cast<int>(_a), static_cast<int>(read8<register8::f>()), static_cast<int>(_b), static_cast<int>(_c),
		static_cast<int>(_d), static_cast<int>(_e), static_cast<int>(_h), static_cast<int>(_l),
		static_cast<int>(_sp), static_cast<int>(_pc), get<cpu_flag::z>() ? 'z' : ' ',
		get<cpu_flag::n>() ? 'n' : ' ', get<cpu_flag::h>() ? 'h' : ' ',
		get<cpu_flag::c>() ? 'c' : ' ');
	debug(buffer);
}

gb::z80_cpu::z80_cpu(gb::memory_map memory, gb::register_file registers) :
	_registers(registers),
	_memory(std::move(memory)),
	_ime(false),
	_halted(false),
	_value8(0xFF),
	_value16(0xFFFF),
	_opcode(nullptr),
	_jumped(false),
	_temp(0),
	_double_speed(false),
	_speed_switch(false)
{
	_memory.add_mapping(this);
}

gb::cputime gb::z80_cpu::fetch_decode_execute()
{
	ASSERT(_opcode == nullptr);

	// interrupts
	if (_ime)
	{
		uint8_t if_ = _memory.read8(internal_ram::if_);
		const uint8_t ie = _memory.read8(internal_ram::ie);
		if (if_ & ie)
		{
			uint16_t pc = _registers.read16<register16::pc>();
			uint16_t sp = _registers.read16<register16::sp>();
			sp -= 2;
			_memory.write16(sp, pc);
			
			for (uint8_t i = 0; i < 5; ++i)
			{
				if ((ie & (1 << i)) && (if_ & (1 << i)))
				{
					pc = 0x0040 + 8 * i;
					if_ &= ~(1 << i);
					_ime = false;
					break;
				}
			}

			_memory.write8(internal_ram::if_, if_);
			_registers.write16<register16::pc>(pc);
			_registers.write16<register16::sp>(sp);
		}
	}

	if (_halted)
	{
		return 4 * clock;
	}

	// fetch
	uint16_t pc = _registers.read16<register16::pc>();
	uint8_t opcode = _memory.read8(pc++);
	_opcode = &(opcode == 0xCB ? cb_opcodes[_memory.read8(pc++)] : opcodes[opcode]);

	// decode
	switch (_opcode->extra_bytes)
	{
	case 0:
		break;
	case 1:
		_value8 = _memory.read8(pc++);
		break;
	case 2:
	{
		uint16_t a = _memory.read8(pc++);
		uint16_t b = _memory.read8(pc++);
		_value16 = a | (b << 8);
		break;
	}
	default:
		ASSERT_UNREACHABLE();
	}

	// increment PC
	_registers.write16<register16::pc>(pc);

	// CAREFUL: HALT will set _opcode to nullptr but never set _jumped
	// this is the reason why I have to read the opcode time before
	// executing the opcode.
	cputime time = _opcode->cycles * (_double_speed ? clock_fast : clock);
	_opcode->base_code(*this);
	if (_jumped)
	{
		_jumped = false;
		time += _opcode->jump_cycles * (_double_speed ? clock_fast : clock);
	}

	return time;
}

gb::cputime gb::z80_cpu::read()
{
	// opcode can be nullptr if the CPU got un-halted by an interrupt
	if (_halted || _opcode == nullptr || _opcode->read_code == nullptr)
	{
		return cputime(0);
	}

	_opcode->read_code(*this);
	return cputime(_double_speed ? clock_fast : clock);
}

gb::cputime gb::z80_cpu::write()
{
	// opcode can be nullptr if the CPU got un-halted by an interrupt
	if (_halted || _opcode == nullptr || _opcode->write_code == nullptr)
	{
		_opcode = nullptr;
		return cputime(0);
	}

	_opcode->write_code(*this);

	_opcode = nullptr;
	return cputime(_double_speed ? clock_fast : clock);
}

void gb::z80_cpu::post_interrupt(interrupt interrupt)
{
	uint8_t if_ = _memory.read8(internal_ram::if_);
	if_ |= static_cast<uint8_t>(interrupt);
	_memory.write8(internal_ram::if_, if_);

	if (_halted && (_memory.read8(internal_ram::ie) & static_cast<uint8_t>(interrupt)) != 0)
	{
		_halted = false;
	}
}

void gb::z80_cpu::stop()
{
	if (_speed_switch)
	{
		_double_speed = !_double_speed;
		_speed_switch = 0;
	}
	else
	{
		// TODO
		debug("STOP not implemented completely!");
	}
}

bool gb::z80_cpu::read8(uint16_t addr, uint8_t &value) const
{
	if (addr == key1)
	{
		value = 0;
		if (_double_speed)
			value |= (1 << 7);
		if (_speed_switch)
			value |= 1;
		return true;
	}
	else
	{
		return false;
	}
}

bool gb::z80_cpu::write8(uint16_t addr, uint8_t value)
{
	if (addr == key1)
	{
		_speed_switch = (value & 1) == 1;
		return true;
	}
	else
	{
		return false;
	}
}

