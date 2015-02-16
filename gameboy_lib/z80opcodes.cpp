/** See http://goldencrystal.free.fr/GBZ80Opcodes.pdf */
#include "z80opcodes.hpp"
#include "z80.hpp"
#include "debug.hpp"
#include "assert.hpp"
#include <string>

using r8 = gb::register8;
using r16 = gb::register16;
using flag = gb::cpu_flag;

namespace
{

uint16_t sign_extend(uint8_t param)
{
	uint16_t offset = param;
	if (offset & 0x0080)
		offset |= 0xFF00;
	return offset;
}

enum class operation
{
	add, adc, sub, sbc, and_, or_, xor_, cp
};

std::string to_string(operation op)
{
	switch (op)
	{
	case operation::add:
		return "ADD";
	case operation::adc:
		return "ADC";
	case operation::sub:
		return "SUB";
	case operation::sbc:
		return "SBC";
	case operation::and_:
		return "AND";
	case operation::or_:
		return "OR";
	case operation::xor_:
		return "XOR";
	case operation::cp:
		return "CP";
	default:
		ASSERT_UNREACHABLE();
	}
}

uint8_t execute_alu(operation op, uint8_t dst, uint8_t src, gb::register_file &rs)
{
	switch (op)
	{
	case operation::add:
	{
		uint8_t result = dst + src;
		rs.set<flag::z>(result == 0);
		rs.set<flag::n>(false);
		rs.set<flag::h>((dst & 0xF) > 0xF - (src & 0xF));
		rs.set<flag::c>(dst > 0xFF - src);
		return result;
	}
	case operation::adc:
	{
		const bool carry = rs.get<flag::c>();
		uint8_t result = execute_alu(operation::add, dst, src + (carry ? 1 : 0), rs);
		if (carry && (src & 0xF) == 0xF)
			rs.set<flag::h>(true);
		if (carry && src == 0xFF)
			rs.set<flag::c>(true);
		return result;
	}
	case operation::sub:
	{
		uint8_t result = dst - src;
		rs.set<flag::z>(result == 0);
		rs.set<flag::n>(true);
		rs.set<flag::h>((dst & 0xF) < (src & 0xF));
		rs.set<flag::c>(dst < src);
		return result;
	}
	case operation::sbc:
	{
		const bool carry = rs.get<flag::c>();
		uint8_t result = execute_alu(operation::sub, dst, src + (carry ? 1 : 0), rs);
		if (carry && (src & 0xF) == 0xF)
			rs.set<flag::h>(true);
		if (carry && src == 0xFF)
			rs.set<flag::c>(true);
		return result;
	}
	case operation::and_:
	{
		uint8_t result = dst & src;
		rs.set<flag::z>(result == 0);
		rs.set<flag::n>(false);
		rs.set<flag::h>(true);
		rs.set<flag::c>(false);
		return result;
	}
	case operation::or_:
	{
		uint8_t result = dst | src;
		rs.set<flag::z>(result == 0);
		rs.set<flag::n>(false);
		rs.set<flag::h>(false);
		rs.set<flag::c>(false);
		return result;
	}
	case operation::xor_:
	{
		uint8_t result = dst ^ src;
		rs.set<flag::z>(result == 0);
		rs.set<flag::n>(false);
		rs.set<flag::h>(false);
		rs.set<flag::c>(false);
		return result;
	}
	case operation::cp:
	{
		execute_alu(operation::sub, dst, src, rs);
		return dst;  // throw away subtraction result
	}
	default:
		ASSERT_UNREACHABLE();
	}
}

enum class cond
{
	nop, nz, z, nc, c
};

std::string to_string(cond c)
{
	switch (c)
	{
	case cond::nop:
		return "";
	case cond::nz:
		return "NZ";
	case cond::z:
		return "Z";
	case cond::nc:
		return "NC";
	case cond::c:
		return "C";
	default:
		ASSERT_UNREACHABLE();
	}
}

bool check_condition(const gb::z80_cpu &cpu, cond c)
{
	auto &rs = cpu.registers();

	switch (c)
	{
	case cond::nop:
		return true;
	case cond::nz:
		return !rs.get<flag::z>();
	case cond::z:
		return rs.get<flag::z>();
	case cond::nc:
		return !rs.get<flag::c>();
	case cond::c:
		return rs.get<flag::c>();
	default:
		ASSERT_UNREACHABLE();
	}
}

// r = register
// i = intermediate
// m = memory denoted by address in register

template <gb::register8 Dst>
class opcode_ld_ri : public gb::opcode
{
public:
	opcode_ld_ri() : gb::opcode("LD " + to_string(Dst) + ",$", 1, 8, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<Dst>(cpu.value8());
	}
};

template <gb::register16 Dst>
class opcode_ld_mi : public gb::opcode
{
public:
	opcode_ld_mi() : gb::opcode("LD (" + to_string(Dst) + "),$", 1, 12, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.memory().write8(cpu.registers().read16<Dst>(), cpu.value8());
	}
};

template <gb::register8 Dst, gb::register8 Src>
class opcode_ld_rr : public gb::opcode
{
public:
	opcode_ld_rr() : gb::opcode("LD " + to_string(Dst) + "," + to_string(Src), 0, 4, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<Dst>(cpu.registers().read8<Src>());
	}
};

template <gb::register8 Dst, gb::register16 Src>
class opcode_ld_rm : public gb::opcode
{
public:
	opcode_ld_rm() : gb::opcode("LD " + to_string(Dst) + ",(" + to_string(Src) + ")", 0, 8, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<Dst>(cpu.memory().read8(cpu.registers().read16<Src>()));
	}
};

template <gb::register8 Dst>
class opcode_ld_rmi : public gb::opcode
{
public:
	opcode_ld_rmi() : gb::opcode("LD " + to_string(Dst) + ",($)", 2, 16, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<Dst>(cpu.memory().read8(cpu.value16()));
	}
};

template <gb::register8 Src>
class opcode_ld_mir : public gb::opcode
{
public:
	opcode_ld_mir() : gb::opcode("LD ($)," + to_string(Src), 2, 16, &execute)	{}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.memory().write8(cpu.value16(), cpu.registers().read8<Src>());
	}
};

template <gb::register16 Dst, gb::register8 Src>
class opcode_ld_mr : public gb::opcode
{
public:
	opcode_ld_mr() : gb::opcode("LD (" + to_string(Dst) + ")," + to_string(Src), 0, 8, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.memory().write8(cpu.registers().read16<Dst>(), cpu.registers().read8<Src>());
	}
};


class opcode_ldff_ac : public gb::opcode
{
public:
	opcode_ldff_ac() : gb::opcode("LD A,(ff00h+C)", 0, 8, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<r8::a>(cpu.memory().read8(0xFF00 + cpu.registers().read8<r8::c>()));
	}
};

class opcode_ldff_ai : public gb::opcode
{
public:
	opcode_ldff_ai() : gb::opcode("LD A,(ff00h+$)", 1, 12, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<r8::a>(cpu.memory().read8(0xFF00 + cpu.value8()));
	}
};

class opcode_ldff_ca : public gb::opcode
{
public:
	opcode_ldff_ca() : gb::opcode("LD (ff00h+C),A", 0, 8, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.memory().write8(0xFF00 + cpu.registers().read8<r8::c>(), cpu.registers().read8<r8::a>());
	}
};

class opcode_ldff_ia : public gb::opcode
{
public:
	opcode_ldff_ia() : gb::opcode("LD (ff00h+$),A", 1, 12, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.memory().write8(0xFF00 + cpu.value8(), cpu.registers().read8<r8::a>());
	}
};

template <bool Dec, bool AHL>
class opcode_lddi : public gb::opcode
{
public:
	opcode_lddi() :
		gb::opcode(std::string(Dec ? "LDD" : "LDI") + " " + (AHL ? "A,(HL)" : "(HL),A"), 0, 8, &execute)
	{}

	static void execute(gb::z80_cpu &cpu)
	{
		uint16_t hl = cpu.registers().read16<r16::hl>();
		
		if (AHL)  // A,(HL)
			cpu.registers().write8<r8::a>(cpu.memory().read8(hl));
		else  // (HL),A
			cpu.memory().write8(hl, cpu.registers().read8<r8::a>());

		if (Dec)
			--hl;
		else  // Inc
			++hl;
		
		cpu.registers().write16<r16::hl>(hl);
	}
};

template <gb::register16 Dst>
class opcode_ld16_ri : public gb::opcode
{
public:
	opcode_ld16_ri() : gb::opcode("LD " + to_string(Dst) + ",$", 2, 12, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write16<Dst>(cpu.value16());
	}
};

template <gb::register16 Dst, gb::register16 Src>
class opcode_ld16_rr : public gb::opcode
{
public:
	opcode_ld16_rr() : gb::opcode("LD " + to_string(Dst) + "," + to_string(Src), 0, 8, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write16<Dst>(cpu.registers().read16<Src>());
	}
};

class opcode_ld16_hlspn : public gb::opcode
{
public:
	opcode_ld16_hlspn() : gb::opcode("LD HL,SP+$", 1, 12, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		const uint16_t sp = cpu.registers().read16<r16::sp>();
		const uint16_t offset = sign_extend(cpu.value8());
		const uint16_t hl = sp + offset;

		cpu.registers().set<flag::z>(false);
		cpu.registers().set<flag::n>(false);
		cpu.registers().set<flag::h>((sp & 0x000F) > 0x000F - (offset & 0x000F));
		cpu.registers().set<flag::c>((sp & 0x00FF) > 0x00FF - (offset & 0x00FF));
		cpu.registers().write16<r16::hl>(hl);
	}
};

template <gb::register16 Src>
class opcode_ld16_mir : public gb::opcode
{
public:
	opcode_ld16_mir() : gb::opcode("LD ($)," + to_string(Src), 2, 20, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.memory().write16(cpu.value16(), cpu.registers().read16<Src>());
	}
};


template <gb::register16 Src>
class opcode_push : public gb::opcode
{
public:
	opcode_push() : gb::opcode("PUSH " + to_string(Src), 0, 16, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		uint16_t sp = cpu.registers().read16<r16::sp>();
		sp -= 2;
		cpu.memory().write16(sp, cpu.registers().read16<Src>());
		cpu.registers().write16<r16::sp>(sp);
	}
};

template <gb::register16 Dst>
class opcode_pop : public gb::opcode
{
public:
	opcode_pop() : gb::opcode("POP " + to_string(Dst), 0, 12, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		uint16_t sp = cpu.registers().read16<r16::sp>();
		cpu.registers().write16<Dst>(cpu.memory().read16(sp));
		sp += 2;
		cpu.registers().write16<r16::sp>(sp);
	}
};

template <operation Op, gb::register8 Dst, gb::register8 Src>
class opcode_alu_rr : public gb::opcode
{
public:
	opcode_alu_rr() : gb::opcode(to_string(Op) + " " + to_string(Dst) + "," + to_string(Src), 0, 4, &execute) {}
		
	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<Dst>(execute_alu(
			Op,
			cpu.registers().read8<Dst>(),
			cpu.registers().read8<Src>(),
			cpu.registers()));
	}
};

template <operation Op, gb::register8 Dst, gb::register16 Src>
class opcode_alu_rm : public gb::opcode
{
public:
	opcode_alu_rm() : gb::opcode(to_string(Op) + " " + to_string(Dst) + ",(" + to_string(Src) + ")", 0, 8, &execute) {}
		
	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<Dst>(execute_alu(
			Op,
			cpu.registers().read8<Dst>(),
			cpu.memory().read8(cpu.registers().read16<Src>()),
			cpu.registers()));
	}
};

template <operation Op, gb::register8 Dst>
class opcode_alu_ri : public gb::opcode
{
public:
	opcode_alu_ri() : gb::opcode(to_string(Op) + " " + to_string(Dst) + ",$", 1, 8, &execute) {}
		
	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<Dst>(execute_alu(
			Op,
			cpu.registers().read8<Dst>(),
			cpu.value8(),
			cpu.registers()));
	}
};


template <bool Dec, gb::register8 Dst>
class opcode_decinc_r : public gb::opcode
{
public:
	opcode_decinc_r() : gb::opcode(std::string(Dec ? "DEC" : "INC") + " " + to_string(Dst), 0, 4, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		uint8_t value = cpu.registers().read8<Dst>();
		if (Dec)
		{
			cpu.registers().set<flag::n>(true);
			cpu.registers().set<flag::h>((value & 0x0F) == 0x00);
			--value;
		}
		else  // Inc
		{
			cpu.registers().set<flag::n>(false);
			cpu.registers().set<flag::h>((value & 0x0F) == 0x0F);
			++value;
		}
		cpu.registers().set<flag::z>(value == 0);
		cpu.registers().write8<Dst>(value);
	}
};

template <bool Dec, gb::register16 Dst>
class opcode_decinc_rm : public gb::opcode
{
public:
	opcode_decinc_rm() : gb::opcode(std::string(Dec ? "DEC" : "INC") + " (" + to_string(Dst) + ")", 0, 12, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		uint8_t value = cpu.memory().read8(cpu.registers().read16<Dst>());
		if (Dec)
		{
			cpu.registers().set<flag::n>(true);
			cpu.registers().set<flag::h>((value & 0x0F) == 0x00);
			--value;
		}
		else  // Inc
		{
			cpu.registers().set<flag::n>(false);
			cpu.registers().set<flag::h>((value & 0x0F) == 0x0F);
			++value;
		}
		cpu.registers().set<flag::z>(value == 0);
		cpu.memory().write8(cpu.registers().read16<Dst>(), value);
	}
};

template <gb::register16 Src>
class opcode_add16_hl : public gb::opcode
{
public:
	opcode_add16_hl() : gb::opcode("ADD HL," + to_string(Src), 0, 8, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		uint16_t hl = cpu.registers().read16<r16::hl>();
		uint16_t offset = cpu.registers().read16<Src>();

		cpu.registers().set<flag::n>(false);
		cpu.registers().set<flag::h>((hl & 0x0FFF) > 0x0FFF - (offset & 0x0FFF));
		cpu.registers().set<flag::c>(hl > 0xFFFF - offset);

		hl += offset;
		cpu.registers().write16<r16::hl>(hl);
	}
};

class opcode_add16_sp_i : public gb::opcode
{
public:
	opcode_add16_sp_i() : gb::opcode("ADD SP,$", 1, 16, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		uint16_t sp = cpu.registers().read16<r16::sp>();
		uint16_t offset = sign_extend(cpu.value8());

		cpu.registers().set<flag::z>(false);
		cpu.registers().set<flag::n>(false);
		cpu.registers().set<flag::h>((sp & 0x000F) > 0x000F - (offset & 0x000F));
		cpu.registers().set<flag::c>((sp & 0x00FF) > 0x00FF - (offset & 0x00FF));

		sp += offset;
		cpu.registers().write16<r16::sp>(sp);
	}
};

template <bool Dec, gb::register16 Dst>
class opcode_decinc16_r : public gb::opcode
{
public:
	opcode_decinc16_r() : gb::opcode(std::string(Dec ? "DEC" : "INC") + " " + to_string(Dst), 0, 8, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		uint16_t value = cpu.registers().read16<Dst>();
		if (Dec)
			--value;
		else  // Inc
			++value;
		cpu.registers().write16<Dst>(value);
	}
};

class opcode_daa : public gb::opcode
{
public:
	opcode_daa() : gb::opcode("DAA", 0, 4, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		// I don't even ...
		// http://forums.nesdev.com/viewtopic.php?t=9088
		// https://courses.engr.illinois.edu/ece390/books/artofasm/CH06/CH06-2.html
		// http://en.wikipedia.org/wiki/Binary-coded_decimal
		// http://www.emutalk.net/threads/41525-Game-Boy/page109

		unsigned int value = cpu.registers().read8<r8::a>();

		if (cpu.registers().get<flag::n>())
		{
			if (cpu.registers().get<flag::h>())
			{
				value = (value - 6) & 0xFF;
			}
			if (cpu.registers().get<flag::c>())
			{
				value -= 0x60;
			}
		}
		else
		{
			if ((value & 0x0F) > 0x09 || cpu.registers().get<flag::h>())
			{
				value += 0x06;
			}
			if (value > 0x9F || cpu.registers().get<flag::c>())
			{
				value += 0x60;
			}
		}
		
		cpu.registers().set<flag::h>(false);
		if ((value & 0x100) == 0x100)
			cpu.registers().set<flag::c>(true);  // do not reset c, if already set!
		value &= 0xFF;
		cpu.registers().set<flag::z>(value == 0);
		cpu.registers().write8<r8::a>(static_cast<uint8_t>(value));
	}
};

class opcode_cpl : public gb::opcode
{
public:
	opcode_cpl() : gb::opcode("CPL", 0, 4, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<r8::a>(~cpu.registers().read8<r8::a>());
		cpu.registers().set<flag::n>(true);
		cpu.registers().set<flag::h>(true);
	}
};

class opcode_ccf : public gb::opcode
{
public:
	opcode_ccf() : gb::opcode("CCF", 0, 4, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().set<flag::n>(false);
		cpu.registers().set<flag::h>(false);
		cpu.registers().set<flag::c>(!cpu.registers().get<flag::c>());
	}
};

class opcode_scf : public gb::opcode
{
public:
	opcode_scf() : gb::opcode("SCF", 0, 4, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().set<flag::n>(false);
		cpu.registers().set<flag::h>(false);
		cpu.registers().set<flag::c>(true);
	}
};

class opcode_halt : public gb::opcode
{
public:
	opcode_halt() : gb::opcode("HALT", 0, 4, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.halt();
	}
};

class opcode_stop : public gb::opcode
{
public:
	opcode_stop() : gb::opcode("STOP", 0, 4, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.stop();
	}
};

class opcode_di : public gb::opcode
{
public:
	opcode_di() : gb::opcode("DI", 0, 4, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.set_ime(false);
	}
};

class opcode_ei : public gb::opcode
{
public:
	opcode_ei() : gb::opcode("EI", 0, 4, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.set_ime(true);
	}
};

class opcode_nop : public gb::opcode
{
public:
	opcode_nop() : gb::opcode("NOP", 0, 4, &execute) {}
	static void execute(gb::z80_cpu &) {}
};

template <bool Left, bool Carry, bool CorrectZ> uint8_t rd_impl(gb::z80_cpu &cpu, uint8_t value)
{
	if (Left)  // Left <<<
	{
		uint8_t bit = value & (1 << 7);
		value <<= 1;
		if (Carry)
		{
			value |= bit >> 7;
		}
		else
		{
			value |= (cpu.registers().get<flag::c>() ? 1 : 0);
		}
		cpu.registers().set<flag::c>(bit != 0);
	}
	else  // Right >>>
	{
		uint8_t bit = value & 1;
		value >>= 1;
		if (Carry)
		{
			value |= bit << 7;
		}
		else
		{
			value |= (cpu.registers().get<flag::c>() ? 1 : 0) << 7;
		}
		cpu.registers().set<flag::c>(bit != 0);
	}

	cpu.registers().set<flag::z>(CorrectZ && value == 0);
	cpu.registers().set<flag::n>(false);
	cpu.registers().set<flag::h>(false);

	return value;
}

template <bool Left, bool Carry>
class opcode_rda : public gb::opcode
{
public:
	opcode_rda() : gb::opcode(std::string("R") + (Left ? "L" : "R") + (Carry ? "C" : "") + "A", 0, 4, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<r8::a>(rd_impl<Left, Carry, false>(cpu, cpu.registers().read8<r8::a>()));
	}
};

template <cond Cond>
class opcode_jp_i : public gb::opcode
{
public:
	opcode_jp_i() : gb::opcode("JP " + to_string(Cond) + (Cond == cond::nop ? "" : ",") + "$", 2, 16, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		if (check_condition(cpu, Cond))
		{
			cpu.registers().write16<r16::pc>(cpu.value16());
		}
	}
};

class opcode_jp_hl : public gb::opcode
{
public:
	opcode_jp_hl() : gb::opcode("JP HL", 0, 4, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write16<r16::pc>(cpu.registers().read16<r16::hl>());
	}
};

template <cond Cond>
class opcode_jr_i : public gb::opcode
{
public:
	opcode_jr_i() : gb::opcode("JR " + to_string(Cond) + (Cond == cond::nop ? "" : ",") + "$", 1, 12, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		if (check_condition(cpu, Cond))
		{
			cpu.registers().write16<r16::pc>(
				cpu.registers().read16<r16::pc>() + static_cast<int8_t>(cpu.value8()));
		}
	}
};

template <cond Cond>
class opcode_call : public gb::opcode
{
public:
	opcode_call() : gb::opcode("CALL " + to_string(Cond) + (Cond == cond::nop ? "" : ",") + "$", 2, 24, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		if (check_condition(cpu, Cond))
		{
			uint16_t pc = cpu.registers().read16<r16::pc>();
			uint16_t sp = cpu.registers().read16<r16::sp>();
			sp -= 2;
			cpu.memory().write16(sp, pc);
			cpu.registers().write16<r16::pc>(cpu.value16());
			cpu.registers().write16<r16::sp>(sp);
		}
	}
};

template <uint8_t Addr>
class opcode_rst : public gb::opcode
{
public:
	opcode_rst() : gb::opcode("RST " + std::to_string(Addr), 0, 16, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		uint16_t pc = cpu.registers().read16<r16::pc>();
		uint16_t sp = cpu.registers().read16<r16::sp>();
		sp -= 2;
		cpu.memory().write16(sp, pc);
		cpu.registers().write16<r16::pc>(Addr);
		cpu.registers().write16<r16::sp>(sp);
	}
};

template <cond Cond, bool Ei>
class opcode_ret : public gb::opcode
{
public:
	opcode_ret() : gb::opcode(std::string("RET") + (Ei ? "I" : "") + (Cond == cond::nop ? std::string("") : (" " + to_string(Cond))), 0, 12, &execute) {}

	static void execute(gb::z80_cpu &cpu)
	{
		if (check_condition(cpu, Cond))
		{
			if (Ei)
				cpu.set_ime(true);

			uint16_t sp = cpu.registers().read16<r16::sp>();
			uint16_t pc = cpu.memory().read16(sp);
			sp += 2;
			cpu.registers().write16<r16::sp>(sp);
			cpu.registers().write16<r16::pc>(pc);
		}
	}
};

class opcode_hang : public gb::opcode
{
public:
	opcode_hang() : gb::opcode("HANG", 0, 0, &execute) {}
	
	static void execute(gb::z80_cpu &cpu)
	{
		debug("WARNING: Game used invalid opcode, hang");
		cpu.set_ime(false);
		cpu.registers().write16<r16::pc>(cpu.registers().read16<r16::pc>() - 1);
	}
};

template <bool Left, bool Carry, gb::register8 Dst>
class opcode_cb_rdc_r : public gb::opcode
{
public:
	opcode_cb_rdc_r() :
		gb::opcode(std::string("R") + (Left ? "L" : "R") + (Carry ? "C" : "") + " " + to_string(Dst), 0, 8, &execute)
	{}
	
	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<Dst>(rd_impl<Left, Carry, true>(cpu, cpu.registers().read8<Dst>()));
	}
};

template <bool Left, bool Carry, gb::register16 Dst>
class opcode_cb_rdc_m : public gb::opcode
{
public:
	opcode_cb_rdc_m() :
		gb::opcode(std::string("R") + (Left ? "L" : "R") + (Carry ? "C" : "") + " (" + to_string(Dst) + ")", 0, 16, &execute)
	{}
	
	static void execute(gb::z80_cpu &cpu)
	{
		uint16_t addr = cpu.registers().read16<Dst>();
		cpu.memory().write8(addr, rd_impl<Left, Carry, true>(cpu, cpu.memory().read8(addr)));
	}
};

template <bool Left> uint8_t sda_impl(gb::z80_cpu &cpu, uint8_t value)
{
	if (Left)
	{
		bool bit = (value & 0x80) != 0;
		value <<= 1;
		cpu.registers().set<flag::c>(bit);
	}
	else  // Right
	{
		uint8_t msb = value & 0x80;
		bool lsb = (value & 1) == 1;
		value >>= 1;
		value |= msb;
		cpu.registers().set<flag::c>(lsb);
	}

	cpu.registers().set<flag::z>(value == 0);
	cpu.registers().set<flag::n>(false);
	cpu.registers().set<flag::h>(false);

	return value;
}

template <bool Left, gb::register8 Dst>
class opcode_cb_sda_r : public gb::opcode
{
public:
	opcode_cb_sda_r() :
		gb::opcode(std::string("S") + (Left ? "L" : "R") + "A " + to_string(Dst), 0, 8, &execute)
	{}
	
	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<Dst>(sda_impl<Left>(cpu, cpu.registers().read8<Dst>()));
	}
};

template <bool Left, gb::register16 Dst>
class opcode_cb_sda_m : public gb::opcode
{
public:
	opcode_cb_sda_m() :
		gb::opcode(std::string("S") + (Left ? "L" : "R") + "A (" + to_string(Dst) + ")", 0, 16, &execute)
	{}
	
	static void execute(gb::z80_cpu &cpu)
	{
		uint16_t addr = cpu.registers().read16<Dst>();
		cpu.memory().write8(addr, sda_impl<Left>(cpu, cpu.memory().read8(addr)));
	}
};

uint8_t swap_impl(gb::z80_cpu &cpu, uint8_t value)
{
	uint8_t low = value & 0x0F;
	value >>= 4;
	value |= low << 4;
	cpu.registers().set<flag::z>(value == 0);
	cpu.registers().set<flag::n>(false);
	cpu.registers().set<flag::h>(false);
	cpu.registers().set<flag::c>(false);
	return value;
}

template <gb::register8 Dst>
class opcode_cb_swap_r : public gb::opcode
{
public:
	opcode_cb_swap_r() :
		gb::opcode("SWAP " + to_string(Dst), 0, 8, &execute)
	{}
	
	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<Dst>(swap_impl(cpu, cpu.registers().read8<Dst>()));
	}
};

template <gb::register16 Dst>
class opcode_cb_swap_m : public gb::opcode
{
public:
	opcode_cb_swap_m() :
		gb::opcode("SWAP (" + to_string(Dst) + ")", 0, 16, &execute)
	{}
	
	static void execute(gb::z80_cpu &cpu)
	{
		uint16_t addr = cpu.registers().read16<Dst>();
		cpu.memory().write8(addr, swap_impl(cpu, cpu.memory().read8(addr)));
	}
};

uint8_t srl_impl(gb::z80_cpu &cpu, uint8_t value)
{
	bool bit = (value & 1) == 1;
	value >>= 1;
	cpu.registers().set<flag::z>(value == 0);
	cpu.registers().set<flag::n>(false);
	cpu.registers().set<flag::h>(false);
	cpu.registers().set<flag::c>(bit);
	return value;
}

template <gb::register8 Dst>
class opcode_cb_srl_r : public gb::opcode
{
public:
	opcode_cb_srl_r() :
		gb::opcode("SRL " + to_string(Dst), 0, 8, &execute)
	{}
	
	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().write8<Dst>(srl_impl(cpu, cpu.registers().read8<Dst>()));
	}
};

template <gb::register16 Dst>
class opcode_cb_srl_m : public gb::opcode
{
public:
	opcode_cb_srl_m() :
		gb::opcode("SRL (" + to_string(Dst) + ")", 0, 16, &execute)
	{}
	
	static void execute(gb::z80_cpu &cpu)
	{
		uint16_t addr = cpu.registers().read16<Dst>();
		cpu.memory().write8(addr, srl_impl(cpu, cpu.memory().read8(addr)));
	}
};

template <uint8_t bit, gb::register8 Dst>
class opcode_cb_bit_r : public gb::opcode
{
public:
	opcode_cb_bit_r() :
		gb::opcode("BIT " + std::to_string(bit) + "," + to_string(Dst), 0, 8, &execute)
	{}
	
	static void execute(gb::z80_cpu &cpu)
	{
		cpu.registers().set<flag::z>((cpu.registers().read8<Dst>() & (1 << bit)) == 0);
		cpu.registers().set<flag::n>(false);
		cpu.registers().set<flag::h>(true);
	}
};

template <uint8_t bit, gb::register16 Dst>
class opcode_cb_bit_m : public gb::opcode
{
public:
	opcode_cb_bit_m() :
		gb::opcode("BIT " + std::to_string(bit) + ",(" + to_string(Dst) + ")", 0, 12, &execute)
	{}
	
	static void execute(gb::z80_cpu &cpu)
	{
		uint16_t addr = cpu.registers().read16<Dst>();
		cpu.registers().set<flag::z>((cpu.memory().read8(addr) & (1 << bit)) == 0);
		cpu.registers().set<flag::n>(false);
		cpu.registers().set<flag::h>(true);
	}
};

template <bool res, uint8_t bit, gb::register8 Dst>
class opcode_cb_resset_r : public gb::opcode
{
public:
	opcode_cb_resset_r() :
		gb::opcode((res ? "RES " : "SET ") + std::to_string(bit) + "," + to_string(Dst), 0, 8, &execute)
	{}
	
	static void execute(gb::z80_cpu &cpu)
	{
		uint8_t b = 1 << bit;
		if (res)
			cpu.registers().write8<Dst>(cpu.registers().read8<Dst>() & ~b);
		else  // set
			cpu.registers().write8<Dst>(cpu.registers().read8<Dst>() | b);
	}
};

template <bool res, uint8_t bit, gb::register16 Dst>
class opcode_cb_resset_m : public gb::opcode
{
public:
	opcode_cb_resset_m() :
		gb::opcode((res ? "RES " : "SET ") + std::to_string(bit) + ",(" + to_string(Dst) + ")", 0, 16, &execute)
	{}
	
	static void execute(gb::z80_cpu &cpu)
	{
		uint8_t b = 1 << bit;
		uint16_t addr = cpu.registers().read16<Dst>();
		if (res)
			cpu.memory().write8(addr, cpu.memory().read8(addr) & ~b);
		else  // set
			cpu.memory().write8(addr, cpu.memory().read8(addr) | b);
	}
};

}

const gb::opcode_table gb::opcodes{{
	/* ops[0x00] = */ opcode_nop(),
	/* ops[0x01] = */ opcode_ld16_ri<r16::bc>(),
	/* ops[0x02] = */ opcode_ld_mr<r16::bc, r8::a>(),
	/* ops[0x03] = */ opcode_decinc16_r<false, r16::bc>(),
	/* ops[0x04] = */ opcode_decinc_r<false, r8::b>(),
	/* ops[0x05] = */ opcode_decinc_r<true, r8::b>(),
	/* ops[0x06] = */ opcode_ld_ri<r8::b>(),
	/* ops[0x07] = */ opcode_rda<true, true>(),
	/* ops[0x08] = */ opcode_ld16_mir<r16::sp>(),
	/* ops[0x09] = */ opcode_add16_hl<r16::bc>(),
	/* ops[0x0A] = */ opcode_ld_rm<r8::a, r16::bc>(),
	/* ops[0x0B] = */ opcode_decinc16_r<true, r16::bc>(),
	/* ops[0x0C] = */ opcode_decinc_r<false, r8::c>(),
	/* ops[0x0D] = */ opcode_decinc_r<true, r8::c>(),
	/* ops[0x0E] = */ opcode_ld_ri<r8::c>(),
	/* ops[0x0F] = */ opcode_rda<false, true>(),
	/* ops[0x10] = */ opcode_stop(),
	/* ops[0x11] = */ opcode_ld16_ri<r16::de>(),
	/* ops[0x12] = */ opcode_ld_mr<r16::de, r8::a>(),
	/* ops[0x13] = */ opcode_decinc16_r<false, r16::de>(),
	/* ops[0x14] = */ opcode_decinc_r<false, r8::d>(),
	/* ops[0x15] = */ opcode_decinc_r<true, r8::d>(),
	/* ops[0x16] = */ opcode_ld_ri<r8::d>(),
	/* ops[0x17] = */ opcode_rda<true, false>(),
	/* ops[0x18] = */ opcode_jr_i<cond::nop>(),
	/* ops[0x19] = */ opcode_add16_hl<r16::de>(),
	/* ops[0x1A] = */ opcode_ld_rm<r8::a, r16::de>(),
	/* ops[0x1B] = */ opcode_decinc16_r<true, r16::de>(),
	/* ops[0x1C] = */ opcode_decinc_r<false, r8::e>(),
	/* ops[0x1D] = */ opcode_decinc_r<true, r8::e>(),
	/* ops[0x1E] = */ opcode_ld_ri<r8::e>(),
	/* ops[0x1F] = */ opcode_rda<false, false>(),
	/* ops[0x20] = */ opcode_jr_i<cond::nz>(),
	/* ops[0x21] = */ opcode_ld16_ri<r16::hl>(),
	/* ops[0x22] = */ opcode_lddi<false, false>(),
	/* ops[0x23] = */ opcode_decinc16_r<false, r16::hl>(),
	/* ops[0x24] = */ opcode_decinc_r<false, r8::h>(),
	/* ops[0x25] = */ opcode_decinc_r<true, r8::h>(),
	/* ops[0x26] = */ opcode_ld_ri<r8::h>(),
	/* ops[0x27] = */ opcode_daa(),
	/* ops[0x28] = */ opcode_jr_i<cond::z>(),
	/* ops[0x29] = */ opcode_add16_hl<r16::hl>(),
	/* ops[0x2A] = */ opcode_lddi<false, true>(),
	/* ops[0x2B] = */ opcode_decinc16_r<true, r16::hl>(),
	/* ops[0x2C] = */ opcode_decinc_r<false, r8::l>(),
	/* ops[0x2D] = */ opcode_decinc_r<true, r8::l>(),
	/* ops[0x2E] = */ opcode_ld_ri<r8::l>(),
	/* ops[0x2F] = */ opcode_cpl(),
	/* ops[0x30] = */ opcode_jr_i<cond::nc>(),
	/* ops[0x31] = */ opcode_ld16_ri<r16::sp>(),
	/* ops[0x32] = */ opcode_lddi<true, false>(),
	/* ops[0x33] = */ opcode_decinc16_r<false, r16::sp>(),
	/* ops[0x34] = */ opcode_decinc_rm<false, r16::hl>(),
	/* ops[0x35] = */ opcode_decinc_rm<true, r16::hl>(),
	/* ops[0x36] = */ opcode_ld_mi<r16::hl>(),
	/* ops[0x37] = */ opcode_scf(),
	/* ops[0x38] = */ opcode_jr_i<cond::c>(),
	/* ops[0x39] = */ opcode_add16_hl<r16::sp>(),
	/* ops[0x3A] = */ opcode_lddi<true, true>(),
	/* ops[0x3B] = */ opcode_decinc16_r<true, r16::sp>(),
	/* ops[0x3C] = */ opcode_decinc_r<false, r8::a>(),
	/* ops[0x3D] = */ opcode_decinc_r<true, r8::a>(),
	/* ops[0x3E] = */ opcode_ld_ri<r8::a>(),
	/* ops[0x3F] = */ opcode_ccf(),
	/* ops[0x40] = */ opcode_ld_rr<r8::b, r8::b>(),
	/* ops[0x41] = */ opcode_ld_rr<r8::b, r8::c>(),
	/* ops[0x42] = */ opcode_ld_rr<r8::b, r8::d>(),
	/* ops[0x43] = */ opcode_ld_rr<r8::b, r8::e>(),
	/* ops[0x44] = */ opcode_ld_rr<r8::b, r8::h>(),
	/* ops[0x45] = */ opcode_ld_rr<r8::b, r8::l>(),
	/* ops[0x46] = */ opcode_ld_rm<r8::b, r16::hl>(),
	/* ops[0x47] = */ opcode_ld_rr<r8::b, r8::a>(),
	/* ops[0x48] = */ opcode_ld_rr<r8::c, r8::b>(),
	/* ops[0x49] = */ opcode_ld_rr<r8::c, r8::c>(),
	/* ops[0x4A] = */ opcode_ld_rr<r8::c, r8::d>(),
	/* ops[0x4B] = */ opcode_ld_rr<r8::c, r8::e>(),
	/* ops[0x4C] = */ opcode_ld_rr<r8::c, r8::h>(),
	/* ops[0x4D] = */ opcode_ld_rr<r8::c, r8::l>(),
	/* ops[0x4E] = */ opcode_ld_rm<r8::c, r16::hl>(),
	/* ops[0x4F] = */ opcode_ld_rr<r8::c, r8::a>(),
	/* ops[0x50] = */ opcode_ld_rr<r8::d, r8::b>(),
	/* ops[0x51] = */ opcode_ld_rr<r8::d, r8::c>(),
	/* ops[0x52] = */ opcode_ld_rr<r8::d, r8::d>(),
	/* ops[0x53] = */ opcode_ld_rr<r8::d, r8::e>(),
	/* ops[0x54] = */ opcode_ld_rr<r8::d, r8::h>(),
	/* ops[0x55] = */ opcode_ld_rr<r8::d, r8::l>(),
	/* ops[0x56] = */ opcode_ld_rm<r8::d, r16::hl>(),
	/* ops[0x57] = */ opcode_ld_rr<r8::d, r8::a>(),
	/* ops[0x58] = */ opcode_ld_rr<r8::e, r8::b>(),
	/* ops[0x59] = */ opcode_ld_rr<r8::e, r8::c>(),
	/* ops[0x5A] = */ opcode_ld_rr<r8::e, r8::d>(),
	/* ops[0x5B] = */ opcode_ld_rr<r8::e, r8::e>(),
	/* ops[0x5C] = */ opcode_ld_rr<r8::e, r8::h>(),
	/* ops[0x5D] = */ opcode_ld_rr<r8::e, r8::l>(),
	/* ops[0x5E] = */ opcode_ld_rm<r8::e, r16::hl>(),
	/* ops[0x5F] = */ opcode_ld_rr<r8::e, r8::a>(),
	/* ops[0x60] = */ opcode_ld_rr<r8::h, r8::b>(),
	/* ops[0x61] = */ opcode_ld_rr<r8::h, r8::c>(),
	/* ops[0x62] = */ opcode_ld_rr<r8::h, r8::d>(),
	/* ops[0x63] = */ opcode_ld_rr<r8::h, r8::e>(),
	/* ops[0x64] = */ opcode_ld_rr<r8::h, r8::h>(),
	/* ops[0x65] = */ opcode_ld_rr<r8::h, r8::l>(),
	/* ops[0x66] = */ opcode_ld_rm<r8::h, r16::hl>(),
	/* ops[0x67] = */ opcode_ld_rr<r8::h, r8::a>(),
	/* ops[0x68] = */ opcode_ld_rr<r8::l, r8::b>(),
	/* ops[0x69] = */ opcode_ld_rr<r8::l, r8::c>(),
	/* ops[0x6A] = */ opcode_ld_rr<r8::l, r8::d>(),
	/* ops[0x6B] = */ opcode_ld_rr<r8::l, r8::e>(),
	/* ops[0x6C] = */ opcode_ld_rr<r8::l, r8::h>(),
	/* ops[0x6D] = */ opcode_ld_rr<r8::l, r8::l>(),
	/* ops[0x6E] = */ opcode_ld_rm<r8::l, r16::hl>(),
	/* ops[0x6F] = */ opcode_ld_rr<r8::l, r8::a>(),
	/* ops[0x70] = */ opcode_ld_mr<r16::hl, r8::b>(),
	/* ops[0x71] = */ opcode_ld_mr<r16::hl, r8::c>(),
	/* ops[0x72] = */ opcode_ld_mr<r16::hl, r8::d>(),
	/* ops[0x73] = */ opcode_ld_mr<r16::hl, r8::e>(),
	/* ops[0x74] = */ opcode_ld_mr<r16::hl, r8::h>(),
	/* ops[0x75] = */ opcode_ld_mr<r16::hl, r8::l>(),
	/* ops[0x76] = */ opcode_halt(),
	/* ops[0x77] = */ opcode_ld_mr<r16::hl, r8::a>(),
	/* ops[0x78] = */ opcode_ld_rr<r8::a, r8::b>(),
	/* ops[0x79] = */ opcode_ld_rr<r8::a, r8::c>(),
	/* ops[0x7A] = */ opcode_ld_rr<r8::a, r8::d>(),
	/* ops[0x7B] = */ opcode_ld_rr<r8::a, r8::e>(),
	/* ops[0x7C] = */ opcode_ld_rr<r8::a, r8::h>(),
	/* ops[0x7D] = */ opcode_ld_rr<r8::a, r8::l>(),
	/* ops[0x7E] = */ opcode_ld_rm<r8::a, r16::hl>(),
	/* ops[0x7F] = */ opcode_ld_rr<r8::a, r8::a>(),
	/* ops[0x80] = */ opcode_alu_rr<operation::add, r8::a, r8::b>(),
	/* ops[0x81] = */ opcode_alu_rr<operation::add, r8::a, r8::c>(),
	/* ops[0x82] = */ opcode_alu_rr<operation::add, r8::a, r8::d>(),
	/* ops[0x83] = */ opcode_alu_rr<operation::add, r8::a, r8::e>(),
	/* ops[0x84] = */ opcode_alu_rr<operation::add, r8::a, r8::h>(),
	/* ops[0x85] = */ opcode_alu_rr<operation::add, r8::a, r8::l>(),
	/* ops[0x86] = */ opcode_alu_rm<operation::add, r8::a, r16::hl>(),
	/* ops[0x87] = */ opcode_alu_rr<operation::add, r8::a, r8::a>(),
	/* ops[0x88] = */ opcode_alu_rr<operation::adc, r8::a, r8::b>(),
	/* ops[0x89] = */ opcode_alu_rr<operation::adc, r8::a, r8::c>(),
	/* ops[0x8A] = */ opcode_alu_rr<operation::adc, r8::a, r8::d>(),
	/* ops[0x8B] = */ opcode_alu_rr<operation::adc, r8::a, r8::e>(),
	/* ops[0x8C] = */ opcode_alu_rr<operation::adc, r8::a, r8::h>(),
	/* ops[0x8D] = */ opcode_alu_rr<operation::adc, r8::a, r8::l>(),
	/* ops[0x8E] = */ opcode_alu_rm<operation::adc, r8::a, r16::hl>(),
	/* ops[0x8F] = */ opcode_alu_rr<operation::adc, r8::a, r8::a>(),
	/* ops[0x90] = */ opcode_alu_rr<operation::sub, r8::a, r8::b>(),
	/* ops[0x91] = */ opcode_alu_rr<operation::sub, r8::a, r8::c>(),
	/* ops[0x92] = */ opcode_alu_rr<operation::sub, r8::a, r8::d>(),
	/* ops[0x93] = */ opcode_alu_rr<operation::sub, r8::a, r8::e>(),
	/* ops[0x94] = */ opcode_alu_rr<operation::sub, r8::a, r8::h>(),
	/* ops[0x95] = */ opcode_alu_rr<operation::sub, r8::a, r8::l>(),
	/* ops[0x96] = */ opcode_alu_rm<operation::sub, r8::a, r16::hl>(),
	/* ops[0x97] = */ opcode_alu_rr<operation::sub, r8::a, r8::a>(),
	/* ops[0x98] = */ opcode_alu_rr<operation::sbc, r8::a, r8::b>(),
	/* ops[0x99] = */ opcode_alu_rr<operation::sbc, r8::a, r8::c>(),
	/* ops[0x9A] = */ opcode_alu_rr<operation::sbc, r8::a, r8::d>(),
	/* ops[0x9B] = */ opcode_alu_rr<operation::sbc, r8::a, r8::e>(),
	/* ops[0x9C] = */ opcode_alu_rr<operation::sbc, r8::a, r8::h>(),
	/* ops[0x9D] = */ opcode_alu_rr<operation::sbc, r8::a, r8::l>(),
	/* ops[0x9E] = */ opcode_alu_rm<operation::sbc, r8::a, r16::hl>(),
	/* ops[0x9F] = */ opcode_alu_rr<operation::sbc, r8::a, r8::a>(),
	/* ops[0xA0] = */ opcode_alu_rr<operation::and_, r8::a, r8::b>(),
	/* ops[0xA1] = */ opcode_alu_rr<operation::and_, r8::a, r8::c>(),
	/* ops[0xA2] = */ opcode_alu_rr<operation::and_, r8::a, r8::d>(),
	/* ops[0xA3] = */ opcode_alu_rr<operation::and_, r8::a, r8::e>(),
	/* ops[0xA4] = */ opcode_alu_rr<operation::and_, r8::a, r8::h>(),
	/* ops[0xA5] = */ opcode_alu_rr<operation::and_, r8::a, r8::l>(),
	/* ops[0xA6] = */ opcode_alu_rm<operation::and_, r8::a, r16::hl>(),
	/* ops[0xA7] = */ opcode_alu_rr<operation::and_, r8::a, r8::a>(),
	/* ops[0xA8] = */ opcode_alu_rr<operation::xor_, r8::a, r8::b>(),
	/* ops[0xA9] = */ opcode_alu_rr<operation::xor_, r8::a, r8::c>(),
	/* ops[0xAA] = */ opcode_alu_rr<operation::xor_, r8::a, r8::d>(),
	/* ops[0xAB] = */ opcode_alu_rr<operation::xor_, r8::a, r8::e>(),
	/* ops[0xAC] = */ opcode_alu_rr<operation::xor_, r8::a, r8::h>(),
	/* ops[0xAD] = */ opcode_alu_rr<operation::xor_, r8::a, r8::l>(),
	/* ops[0xAE] = */ opcode_alu_rm<operation::xor_, r8::a, r16::hl>(),
	/* ops[0xAF] = */ opcode_alu_rr<operation::xor_, r8::a, r8::a>(),
	/* ops[0xB0] = */ opcode_alu_rr<operation::or_, r8::a, r8::b>(),
	/* ops[0xB1] = */ opcode_alu_rr<operation::or_, r8::a, r8::c>(),
	/* ops[0xB2] = */ opcode_alu_rr<operation::or_, r8::a, r8::d>(),
	/* ops[0xB3] = */ opcode_alu_rr<operation::or_, r8::a, r8::e>(),
	/* ops[0xB4] = */ opcode_alu_rr<operation::or_, r8::a, r8::h>(),
	/* ops[0xB5] = */ opcode_alu_rr<operation::or_, r8::a, r8::l>(),
	/* ops[0xB6] = */ opcode_alu_rm<operation::or_, r8::a, r16::hl>(),
	/* ops[0xB7] = */ opcode_alu_rr<operation::or_, r8::a, r8::a>(),
	/* ops[0xB8] = */ opcode_alu_rr<operation::cp, r8::a, r8::b>(),
	/* ops[0xB9] = */ opcode_alu_rr<operation::cp, r8::a, r8::c>(),
	/* ops[0xBA] = */ opcode_alu_rr<operation::cp, r8::a, r8::d>(),
	/* ops[0xBB] = */ opcode_alu_rr<operation::cp, r8::a, r8::e>(),
	/* ops[0xBC] = */ opcode_alu_rr<operation::cp, r8::a, r8::h>(),
	/* ops[0xBD] = */ opcode_alu_rr<operation::cp, r8::a, r8::l>(),
	/* ops[0xBE] = */ opcode_alu_rm<operation::cp, r8::a, r16::hl>(),
	/* ops[0xBF] = */ opcode_alu_rr<operation::cp, r8::a, r8::a>(),
	/* ops[0xC0] = */ opcode_ret<cond::nz, false>(),
	/* ops[0xC1] = */ opcode_pop<r16::bc>(),
	/* ops[0xC2] = */ opcode_jp_i<cond::nz>(),
	/* ops[0xC3] = */ opcode_jp_i<cond::nop>(),
	/* ops[0xC4] = */ opcode_call<cond::nz>(),
	/* ops[0xC5] = */ opcode_push<r16::bc>(),
	/* ops[0xC6] = */ opcode_alu_ri<operation::add, r8::a>(),
	/* ops[0xC7] = */ opcode_rst<0x00>(),
	/* ops[0xC8] = */ opcode_ret<cond::z, false>(),
	/* ops[0xC9] = */ opcode_ret<cond::nop, false>(),
	/* ops[0xCA] = */ opcode_jp_i<cond::z>(),
	/* ops[0xCB] = */ opcode_hang(),
	/* ops[0xCC] = */ opcode_call<cond::z>(),
	/* ops[0xCD] = */ opcode_call<cond::nop>(),
	/* ops[0xCE] = */ opcode_alu_ri<operation::adc, r8::a>(),
	/* ops[0xCF] = */ opcode_rst<0x08>(),
	/* ops[0xD0] = */ opcode_ret<cond::nc, false>(),
	/* ops[0xD1] = */ opcode_pop<r16::de>(),
	/* ops[0xD2] = */ opcode_jp_i<cond::nc>(),
	/* ops[0xD3] = */ opcode_hang(),
	/* ops[0xD4] = */ opcode_call<cond::nc>(),
	/* ops[0xD5] = */ opcode_push<r16::de>(),
	/* ops[0xD6] = */ opcode_alu_ri<operation::sub, r8::a>(),
	/* ops[0xD7] = */ opcode_rst<0x10>(),
	/* ops[0xD8] = */ opcode_ret<cond::c, false>(),
	/* ops[0xD9] = */ opcode_ret<cond::nop, true>(),
	/* ops[0xDA] = */ opcode_jp_i<cond::c>(),
	/* ops[0xDB] = */ opcode_hang(),
	/* ops[0xDC] = */ opcode_call<cond::c>(),
	/* ops[0xDD] = */ opcode_hang(),
	/* ops[0xDE] = */ opcode_alu_ri<operation::sbc, r8::a>(),
	/* ops[0xDF] = */ opcode_rst<0x18>(),
	/* ops[0xE0] = */ opcode_ldff_ia(),
	/* ops[0xE1] = */ opcode_pop<r16::hl>(),
	/* ops[0xE2] = */ opcode_ldff_ca(),
	/* ops[0xE3] = */ opcode_hang(),
	/* ops[0xE4] = */ opcode_hang(),
	/* ops[0xE5] = */ opcode_push<r16::hl>(),
	/* ops[0xE6] = */ opcode_alu_ri<operation::and_, r8::a>(),
	/* ops[0xE7] = */ opcode_rst<0x20>(),
	/* ops[0xE8] = */ opcode_add16_sp_i(),
	/* ops[0xE9] = */ opcode_jp_hl(),
	/* ops[0xEA] = */ opcode_ld_mir<r8::a>(),
	/* ops[0xEB] = */ opcode_hang(),
	/* ops[0xEC] = */ opcode_hang(),
	/* ops[0xED] = */ opcode_hang(),
	/* ops[0xEE] = */ opcode_alu_ri<operation::xor_, r8::a>(),
	/* ops[0xEF] = */ opcode_rst<0x28>(),
	/* ops[0xF0] = */ opcode_ldff_ai(),
	/* ops[0xF1] = */ opcode_pop<r16::af>(),
	/* ops[0xF2] = */ opcode_ldff_ac(),
	/* ops[0xF3] = */ opcode_di(),
	/* ops[0xF4] = */ opcode_hang(),
	/* ops[0xF5] = */ opcode_push<r16::af>(),
	/* ops[0xF6] = */ opcode_alu_ri<operation::or_, r8::a>(),
	/* ops[0xF7] = */ opcode_rst<0x30>(),
	/* ops[0xF8] = */ opcode_ld16_hlspn(),
	/* ops[0xF9] = */ opcode_ld16_rr<r16::sp, r16::hl>(),
	/* ops[0xFA] = */ opcode_ld_rmi<r8::a>(),
	/* ops[0xFB] = */ opcode_ei(),
	/* ops[0xFC] = */ opcode_hang(),
	/* ops[0xFD] = */ opcode_hang(),
	/* ops[0xFE] = */ opcode_alu_ri<operation::cp, r8::a>(),
	/* ops[0xFF] = */ opcode_rst<0x38>()
}};

const gb::opcode_table gb::cb_opcodes{{
	/* cb[0x00] = */ opcode_cb_rdc_r<true, true, r8::b>(),
	/* cb[0x01] = */ opcode_cb_rdc_r<true, true, r8::c>(),
	/* cb[0x02] = */ opcode_cb_rdc_r<true, true, r8::d>(),
	/* cb[0x03] = */ opcode_cb_rdc_r<true, true, r8::e>(),
	/* cb[0x04] = */ opcode_cb_rdc_r<true, true, r8::h>(),
	/* cb[0x05] = */ opcode_cb_rdc_r<true, true, r8::l>(),
	/* cb[0x06] = */ opcode_cb_rdc_m<true, true, r16::hl>(),
	/* cb[0x07] = */ opcode_cb_rdc_r<true, true, r8::a>(),
	/* cb[0x08] = */ opcode_cb_rdc_r<false, true, r8::b>(),
	/* cb[0x09] = */ opcode_cb_rdc_r<false, true, r8::c>(),
	/* cb[0x0A] = */ opcode_cb_rdc_r<false, true, r8::d>(),
	/* cb[0x0B] = */ opcode_cb_rdc_r<false, true, r8::e>(),
	/* cb[0x0C] = */ opcode_cb_rdc_r<false, true, r8::h>(),
	/* cb[0x0D] = */ opcode_cb_rdc_r<false, true, r8::l>(),
	/* cb[0x0E] = */ opcode_cb_rdc_m<false, true, r16::hl>(),
	/* cb[0x0F] = */ opcode_cb_rdc_r<false, true, r8::a>(),
	/* cb[0x10] = */ opcode_cb_rdc_r<true, false, r8::b>(),
	/* cb[0x11] = */ opcode_cb_rdc_r<true, false, r8::c>(),
	/* cb[0x12] = */ opcode_cb_rdc_r<true, false, r8::d>(),
	/* cb[0x13] = */ opcode_cb_rdc_r<true, false, r8::e>(),
	/* cb[0x14] = */ opcode_cb_rdc_r<true, false, r8::h>(),
	/* cb[0x15] = */ opcode_cb_rdc_r<true, false, r8::l>(),
	/* cb[0x16] = */ opcode_cb_rdc_m<true, false, r16::hl>(),
	/* cb[0x17] = */ opcode_cb_rdc_r<true, false, r8::a>(),
	/* cb[0x18] = */ opcode_cb_rdc_r<false, false, r8::b>(),
	/* cb[0x19] = */ opcode_cb_rdc_r<false, false, r8::c>(),
	/* cb[0x1A] = */ opcode_cb_rdc_r<false, false, r8::d>(),
	/* cb[0x1B] = */ opcode_cb_rdc_r<false, false, r8::e>(),
	/* cb[0x1C] = */ opcode_cb_rdc_r<false, false, r8::h>(),
	/* cb[0x1D] = */ opcode_cb_rdc_r<false, false, r8::l>(),
	/* cb[0x1E] = */ opcode_cb_rdc_m<false, false, r16::hl>(),
	/* cb[0x1F] = */ opcode_cb_rdc_r<false, false, r8::a>(),
	/* cb[0x20] = */ opcode_cb_sda_r<true, r8::b>(),
	/* cb[0x21] = */ opcode_cb_sda_r<true, r8::c>(),
	/* cb[0x22] = */ opcode_cb_sda_r<true, r8::d>(),
	/* cb[0x23] = */ opcode_cb_sda_r<true, r8::e>(),
	/* cb[0x24] = */ opcode_cb_sda_r<true, r8::h>(),
	/* cb[0x25] = */ opcode_cb_sda_r<true, r8::l>(),
	/* cb[0x26] = */ opcode_cb_sda_m<true, r16::hl>(),
	/* cb[0x27] = */ opcode_cb_sda_r<true, r8::a>(),
	/* cb[0x28] = */ opcode_cb_sda_r<false, r8::b>(),
	/* cb[0x29] = */ opcode_cb_sda_r<false, r8::c>(),
	/* cb[0x2A] = */ opcode_cb_sda_r<false, r8::d>(),
	/* cb[0x2B] = */ opcode_cb_sda_r<false, r8::e>(),
	/* cb[0x2C] = */ opcode_cb_sda_r<false, r8::h>(),
	/* cb[0x2D] = */ opcode_cb_sda_r<false, r8::l>(),
	/* cb[0x2E] = */ opcode_cb_sda_m<false, r16::hl>(),
	/* cb[0x2F] = */ opcode_cb_sda_r<false, r8::a>(),
	/* cb[0x30] = */ opcode_cb_swap_r<r8::b>(),
	/* cb[0x31] = */ opcode_cb_swap_r<r8::c>(),
	/* cb[0x32] = */ opcode_cb_swap_r<r8::d>(),
	/* cb[0x33] = */ opcode_cb_swap_r<r8::e>(),
	/* cb[0x34] = */ opcode_cb_swap_r<r8::h>(),
	/* cb[0x35] = */ opcode_cb_swap_r<r8::l>(),
	/* cb[0x36] = */ opcode_cb_swap_m<r16::hl>(),
	/* cb[0x37] = */ opcode_cb_swap_r<r8::a>(),
	/* cb[0x38] = */ opcode_cb_srl_r<r8::b>(),
	/* cb[0x39] = */ opcode_cb_srl_r<r8::c>(),
	/* cb[0x3A] = */ opcode_cb_srl_r<r8::d>(),
	/* cb[0x3B] = */ opcode_cb_srl_r<r8::e>(),
	/* cb[0x3C] = */ opcode_cb_srl_r<r8::h>(),
	/* cb[0x3D] = */ opcode_cb_srl_r<r8::l>(),
	/* cb[0x3E] = */ opcode_cb_srl_m<r16::hl>(),
	/* cb[0x3F] = */ opcode_cb_srl_r<r8::a>(),
	/* cb[0x40] = */ opcode_cb_bit_r<0, r8::b>(),
	/* cb[0x41] = */ opcode_cb_bit_r<0, r8::c>(),
	/* cb[0x42] = */ opcode_cb_bit_r<0, r8::d>(),
	/* cb[0x43] = */ opcode_cb_bit_r<0, r8::e>(),
	/* cb[0x44] = */ opcode_cb_bit_r<0, r8::h>(),
	/* cb[0x45] = */ opcode_cb_bit_r<0, r8::l>(),
	/* cb[0x46] = */ opcode_cb_bit_m<0, r16::hl>(),
	/* cb[0x47] = */ opcode_cb_bit_r<0, r8::a>(),
	/* cb[0x48] = */ opcode_cb_bit_r<1, r8::b>(),
	/* cb[0x49] = */ opcode_cb_bit_r<1, r8::c>(),
	/* cb[0x4A] = */ opcode_cb_bit_r<1, r8::d>(),
	/* cb[0x4B] = */ opcode_cb_bit_r<1, r8::e>(),
	/* cb[0x4C] = */ opcode_cb_bit_r<1, r8::h>(),
	/* cb[0x4D] = */ opcode_cb_bit_r<1, r8::l>(),
	/* cb[0x4E] = */ opcode_cb_bit_m<1, r16::hl>(),
	/* cb[0x4F] = */ opcode_cb_bit_r<1, r8::a>(),
	/* cb[0x50] = */ opcode_cb_bit_r<2, r8::b>(),
	/* cb[0x51] = */ opcode_cb_bit_r<2, r8::c>(),
	/* cb[0x52] = */ opcode_cb_bit_r<2, r8::d>(),
	/* cb[0x53] = */ opcode_cb_bit_r<2, r8::e>(),
	/* cb[0x54] = */ opcode_cb_bit_r<2, r8::h>(),
	/* cb[0x55] = */ opcode_cb_bit_r<2, r8::l>(),
	/* cb[0x56] = */ opcode_cb_bit_m<2, r16::hl>(),
	/* cb[0x57] = */ opcode_cb_bit_r<2, r8::a>(),
	/* cb[0x58] = */ opcode_cb_bit_r<3, r8::b>(),
	/* cb[0x59] = */ opcode_cb_bit_r<3, r8::c>(),
	/* cb[0x5A] = */ opcode_cb_bit_r<3, r8::d>(),
	/* cb[0x5B] = */ opcode_cb_bit_r<3, r8::e>(),
	/* cb[0x5C] = */ opcode_cb_bit_r<3, r8::h>(),
	/* cb[0x5D] = */ opcode_cb_bit_r<3, r8::l>(),
	/* cb[0x5E] = */ opcode_cb_bit_m<3, r16::hl>(),
	/* cb[0x5F] = */ opcode_cb_bit_r<3, r8::a>(),
	/* cb[0x60] = */ opcode_cb_bit_r<4, r8::b>(),
	/* cb[0x61] = */ opcode_cb_bit_r<4, r8::c>(),
	/* cb[0x62] = */ opcode_cb_bit_r<4, r8::d>(),
	/* cb[0x63] = */ opcode_cb_bit_r<4, r8::e>(),
	/* cb[0x64] = */ opcode_cb_bit_r<4, r8::h>(),
	/* cb[0x65] = */ opcode_cb_bit_r<4, r8::l>(),
	/* cb[0x66] = */ opcode_cb_bit_m<4, r16::hl>(),
	/* cb[0x67] = */ opcode_cb_bit_r<4, r8::a>(),
	/* cb[0x68] = */ opcode_cb_bit_r<5, r8::b>(),
	/* cb[0x69] = */ opcode_cb_bit_r<5, r8::c>(),
	/* cb[0x6A] = */ opcode_cb_bit_r<5, r8::d>(),
	/* cb[0x6B] = */ opcode_cb_bit_r<5, r8::e>(),
	/* cb[0x6C] = */ opcode_cb_bit_r<5, r8::h>(),
	/* cb[0x6D] = */ opcode_cb_bit_r<5, r8::l>(),
	/* cb[0x6E] = */ opcode_cb_bit_m<5, r16::hl>(),
	/* cb[0x6F] = */ opcode_cb_bit_r<5, r8::a>(),
	/* cb[0x70] = */ opcode_cb_bit_r<6, r8::b>(),
	/* cb[0x71] = */ opcode_cb_bit_r<6, r8::c>(),
	/* cb[0x72] = */ opcode_cb_bit_r<6, r8::d>(),
	/* cb[0x73] = */ opcode_cb_bit_r<6, r8::e>(),
	/* cb[0x74] = */ opcode_cb_bit_r<6, r8::h>(),
	/* cb[0x75] = */ opcode_cb_bit_r<6, r8::l>(),
	/* cb[0x76] = */ opcode_cb_bit_m<6, r16::hl>(),
	/* cb[0x77] = */ opcode_cb_bit_r<6, r8::a>(),
	/* cb[0x78] = */ opcode_cb_bit_r<7, r8::b>(),
	/* cb[0x79] = */ opcode_cb_bit_r<7, r8::c>(),
	/* cb[0x7A] = */ opcode_cb_bit_r<7, r8::d>(),
	/* cb[0x7B] = */ opcode_cb_bit_r<7, r8::e>(),
	/* cb[0x7C] = */ opcode_cb_bit_r<7, r8::h>(),
	/* cb[0x7D] = */ opcode_cb_bit_r<7, r8::l>(),
	/* cb[0x7E] = */ opcode_cb_bit_m<7, r16::hl>(),
	/* cb[0x7F] = */ opcode_cb_bit_r<7, r8::a>(),
	/* cb[0x80] = */ opcode_cb_resset_r<true, 0, r8::b>(),
	/* cb[0x81] = */ opcode_cb_resset_r<true, 0, r8::c>(),
	/* cb[0x82] = */ opcode_cb_resset_r<true, 0, r8::d>(),
	/* cb[0x83] = */ opcode_cb_resset_r<true, 0, r8::e>(),
	/* cb[0x84] = */ opcode_cb_resset_r<true, 0, r8::h>(),
	/* cb[0x85] = */ opcode_cb_resset_r<true, 0, r8::l>(),
	/* cb[0x86] = */ opcode_cb_resset_m<true, 0, r16::hl>(),
	/* cb[0x87] = */ opcode_cb_resset_r<true, 0, r8::a>(),
	/* cb[0x88] = */ opcode_cb_resset_r<true, 1, r8::b>(),
	/* cb[0x89] = */ opcode_cb_resset_r<true, 1, r8::c>(),
	/* cb[0x8A] = */ opcode_cb_resset_r<true, 1, r8::d>(),
	/* cb[0x8B] = */ opcode_cb_resset_r<true, 1, r8::e>(),
	/* cb[0x8C] = */ opcode_cb_resset_r<true, 1, r8::h>(),
	/* cb[0x8D] = */ opcode_cb_resset_r<true, 1, r8::l>(),
	/* cb[0x8E] = */ opcode_cb_resset_m<true, 1, r16::hl>(),
	/* cb[0x8F] = */ opcode_cb_resset_r<true, 1, r8::a>(),
	/* cb[0x90] = */ opcode_cb_resset_r<true, 2, r8::b>(),
	/* cb[0x91] = */ opcode_cb_resset_r<true, 2, r8::c>(),
	/* cb[0x92] = */ opcode_cb_resset_r<true, 2, r8::d>(),
	/* cb[0x93] = */ opcode_cb_resset_r<true, 2, r8::e>(),
	/* cb[0x94] = */ opcode_cb_resset_r<true, 2, r8::h>(),
	/* cb[0x95] = */ opcode_cb_resset_r<true, 2, r8::l>(),
	/* cb[0x96] = */ opcode_cb_resset_m<true, 2, r16::hl>(),
	/* cb[0x97] = */ opcode_cb_resset_r<true, 2, r8::a>(),
	/* cb[0x98] = */ opcode_cb_resset_r<true, 3, r8::b>(),
	/* cb[0x99] = */ opcode_cb_resset_r<true, 3, r8::c>(),
	/* cb[0x9A] = */ opcode_cb_resset_r<true, 3, r8::d>(),
	/* cb[0x9B] = */ opcode_cb_resset_r<true, 3, r8::e>(),
	/* cb[0x9C] = */ opcode_cb_resset_r<true, 3, r8::h>(),
	/* cb[0x9D] = */ opcode_cb_resset_r<true, 3, r8::l>(),
	/* cb[0x9E] = */ opcode_cb_resset_m<true, 3, r16::hl>(),
	/* cb[0x9F] = */ opcode_cb_resset_r<true, 3, r8::a>(),
	/* cb[0xA0] = */ opcode_cb_resset_r<true, 4, r8::b>(),
	/* cb[0xA1] = */ opcode_cb_resset_r<true, 4, r8::c>(),
	/* cb[0xA2] = */ opcode_cb_resset_r<true, 4, r8::d>(),
	/* cb[0xA3] = */ opcode_cb_resset_r<true, 4, r8::e>(),
	/* cb[0xA4] = */ opcode_cb_resset_r<true, 4, r8::h>(),
	/* cb[0xA5] = */ opcode_cb_resset_r<true, 4, r8::l>(),
	/* cb[0xA6] = */ opcode_cb_resset_m<true, 4, r16::hl>(),
	/* cb[0xA7] = */ opcode_cb_resset_r<true, 4, r8::a>(),
	/* cb[0xA8] = */ opcode_cb_resset_r<true, 5, r8::b>(),
	/* cb[0xA9] = */ opcode_cb_resset_r<true, 5, r8::c>(),
	/* cb[0xAA] = */ opcode_cb_resset_r<true, 5, r8::d>(),
	/* cb[0xAB] = */ opcode_cb_resset_r<true, 5, r8::e>(),
	/* cb[0xAC] = */ opcode_cb_resset_r<true, 5, r8::h>(),
	/* cb[0xAD] = */ opcode_cb_resset_r<true, 5, r8::l>(),
	/* cb[0xAE] = */ opcode_cb_resset_m<true, 5, r16::hl>(),
	/* cb[0xAF] = */ opcode_cb_resset_r<true, 5, r8::a>(),
	/* cb[0xB0] = */ opcode_cb_resset_r<true, 6, r8::b>(),
	/* cb[0xB1] = */ opcode_cb_resset_r<true, 6, r8::c>(),
	/* cb[0xB2] = */ opcode_cb_resset_r<true, 6, r8::d>(),
	/* cb[0xB3] = */ opcode_cb_resset_r<true, 6, r8::e>(),
	/* cb[0xB4] = */ opcode_cb_resset_r<true, 6, r8::h>(),
	/* cb[0xB5] = */ opcode_cb_resset_r<true, 6, r8::l>(),
	/* cb[0xB6] = */ opcode_cb_resset_m<true, 6, r16::hl>(),
	/* cb[0xB7] = */ opcode_cb_resset_r<true, 6, r8::a>(),
	/* cb[0xB8] = */ opcode_cb_resset_r<true, 7, r8::b>(),
	/* cb[0xB9] = */ opcode_cb_resset_r<true, 7, r8::c>(),
	/* cb[0xBA] = */ opcode_cb_resset_r<true, 7, r8::d>(),
	/* cb[0xBB] = */ opcode_cb_resset_r<true, 7, r8::e>(),
	/* cb[0xBC] = */ opcode_cb_resset_r<true, 7, r8::h>(),
	/* cb[0xBD] = */ opcode_cb_resset_r<true, 7, r8::l>(),
	/* cb[0xBE] = */ opcode_cb_resset_m<true, 7, r16::hl>(),
	/* cb[0xBF] = */ opcode_cb_resset_r<true, 7, r8::a>(),
	/* cb[0xC0] = */ opcode_cb_resset_r<false, 0, r8::b>(),
	/* cb[0xC1] = */ opcode_cb_resset_r<false, 0, r8::c>(),
	/* cb[0xC2] = */ opcode_cb_resset_r<false, 0, r8::d>(),
	/* cb[0xC3] = */ opcode_cb_resset_r<false, 0, r8::e>(),
	/* cb[0xC4] = */ opcode_cb_resset_r<false, 0, r8::h>(),
	/* cb[0xC5] = */ opcode_cb_resset_r<false, 0, r8::l>(),
	/* cb[0xC6] = */ opcode_cb_resset_m<false, 0, r16::hl>(),
	/* cb[0xC7] = */ opcode_cb_resset_r<false, 0, r8::a>(),
	/* cb[0xC8] = */ opcode_cb_resset_r<false, 1, r8::b>(),
	/* cb[0xC9] = */ opcode_cb_resset_r<false, 1, r8::c>(),
	/* cb[0xCA] = */ opcode_cb_resset_r<false, 1, r8::d>(),
	/* cb[0xCB] = */ opcode_cb_resset_r<false, 1, r8::e>(),
	/* cb[0xCC] = */ opcode_cb_resset_r<false, 1, r8::h>(),
	/* cb[0xCD] = */ opcode_cb_resset_r<false, 1, r8::l>(),
	/* cb[0xCE] = */ opcode_cb_resset_m<false, 1, r16::hl>(),
	/* cb[0xCF] = */ opcode_cb_resset_r<false, 1, r8::a>(),
	/* cb[0xD0] = */ opcode_cb_resset_r<false, 2, r8::b>(),
	/* cb[0xD1] = */ opcode_cb_resset_r<false, 2, r8::c>(),
	/* cb[0xD2] = */ opcode_cb_resset_r<false, 2, r8::d>(),
	/* cb[0xD3] = */ opcode_cb_resset_r<false, 2, r8::e>(),
	/* cb[0xD4] = */ opcode_cb_resset_r<false, 2, r8::h>(),
	/* cb[0xD5] = */ opcode_cb_resset_r<false, 2, r8::l>(),
	/* cb[0xD6] = */ opcode_cb_resset_m<false, 2, r16::hl>(),
	/* cb[0xD7] = */ opcode_cb_resset_r<false, 2, r8::a>(),
	/* cb[0xD8] = */ opcode_cb_resset_r<false, 3, r8::b>(),
	/* cb[0xD9] = */ opcode_cb_resset_r<false, 3, r8::c>(),
	/* cb[0xDA] = */ opcode_cb_resset_r<false, 3, r8::d>(),
	/* cb[0xDB] = */ opcode_cb_resset_r<false, 3, r8::e>(),
	/* cb[0xDC] = */ opcode_cb_resset_r<false, 3, r8::h>(),
	/* cb[0xDD] = */ opcode_cb_resset_r<false, 3, r8::l>(),
	/* cb[0xDE] = */ opcode_cb_resset_m<false, 3, r16::hl>(),
	/* cb[0xDF] = */ opcode_cb_resset_r<false, 3, r8::a>(),
	/* cb[0xE0] = */ opcode_cb_resset_r<false, 4, r8::b>(),
	/* cb[0xE1] = */ opcode_cb_resset_r<false, 4, r8::c>(),
	/* cb[0xE2] = */ opcode_cb_resset_r<false, 4, r8::d>(),
	/* cb[0xE3] = */ opcode_cb_resset_r<false, 4, r8::e>(),
	/* cb[0xE4] = */ opcode_cb_resset_r<false, 4, r8::h>(),
	/* cb[0xE5] = */ opcode_cb_resset_r<false, 4, r8::l>(),
	/* cb[0xE6] = */ opcode_cb_resset_m<false, 4, r16::hl>(),
	/* cb[0xE7] = */ opcode_cb_resset_r<false, 4, r8::a>(),
	/* cb[0xE8] = */ opcode_cb_resset_r<false, 5, r8::b>(),
	/* cb[0xE9] = */ opcode_cb_resset_r<false, 5, r8::c>(),
	/* cb[0xEA] = */ opcode_cb_resset_r<false, 5, r8::d>(),
	/* cb[0xEB] = */ opcode_cb_resset_r<false, 5, r8::e>(),
	/* cb[0xEC] = */ opcode_cb_resset_r<false, 5, r8::h>(),
	/* cb[0xED] = */ opcode_cb_resset_r<false, 5, r8::l>(),
	/* cb[0xEE] = */ opcode_cb_resset_m<false, 5, r16::hl>(),
	/* cb[0xEF] = */ opcode_cb_resset_r<false, 5, r8::a>(),
	/* cb[0xF0] = */ opcode_cb_resset_r<false, 6, r8::b>(),
	/* cb[0xF1] = */ opcode_cb_resset_r<false, 6, r8::c>(),
	/* cb[0xF2] = */ opcode_cb_resset_r<false, 6, r8::d>(),
	/* cb[0xF3] = */ opcode_cb_resset_r<false, 6, r8::e>(),
	/* cb[0xF4] = */ opcode_cb_resset_r<false, 6, r8::h>(),
	/* cb[0xF5] = */ opcode_cb_resset_r<false, 6, r8::l>(),
	/* cb[0xF6] = */ opcode_cb_resset_m<false, 6, r16::hl>(),
	/* cb[0xF7] = */ opcode_cb_resset_r<false, 6, r8::a>(),
	/* cb[0xF8] = */ opcode_cb_resset_r<false, 7, r8::b>(),
	/* cb[0xF9] = */ opcode_cb_resset_r<false, 7, r8::c>(),
	/* cb[0xFA] = */ opcode_cb_resset_r<false, 7, r8::d>(),
	/* cb[0xFB] = */ opcode_cb_resset_r<false, 7, r8::e>(),
	/* cb[0xFC] = */ opcode_cb_resset_r<false, 7, r8::h>(),
	/* cb[0xFD] = */ opcode_cb_resset_r<false, 7, r8::l>(),
	/* cb[0xFE] = */ opcode_cb_resset_m<false, 7, r16::hl>(),
	/* cb[0xFF] = */ opcode_cb_resset_r<false, 7, r8::a>()
}};
