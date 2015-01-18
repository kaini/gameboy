/** See http://goldencrystal.free.fr/GBZ80Opcodes.pdf */
#include "z80opcodes.hpp"
#include "z80.hpp"
#include "debug.hpp"
#include <string>
#include <cassert>

namespace
{

using r8 = gb::register8;
using r16 = gb::register16;
using flag = gb::cpu_flag;

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
		assert(false);
		return {};
	}
}

uint8_t execute_alu(operation op, uint8_t dst, uint8_t src, gb::register_file &rs)
{
	uint8_t result;
	switch (op)
	{
	case operation::add:
		result = dst + src;
		rs.set<flag::z>(result == 0);
		rs.set<flag::n>(false);
		rs.set<flag::h>((dst & 0xF) > 0xF - (src & 0xF));
		rs.set<flag::c>(dst > 0xFF - src);
		break;
	case operation::adc:
		result = dst + src + (rs.get<flag::c>() ? 1 : 0);
		rs.set<flag::z>(result == 0);
		rs.set<flag::n>(false);
		rs.set<flag::h>((dst & 0xF) > 0xF - (src & 0xF) - 1);
		rs.set<flag::c>(dst > 0xFF - src - 1);
		break;
	case operation::sub:
		result = execute_alu(operation::add, dst, ~src + 1, rs);
		rs.set<flag::n>(true);
		break;
	case operation::sbc:
		result = execute_alu(operation::adc, dst, ~(src + (rs.get<flag::c>() ? 1 : 0)) + 1, rs);
		rs.set<flag::n>(true);
		break;
	case operation::and_:
		result = dst & src;
		rs.set<flag::z>(result == 0);
		rs.set<flag::n>(false);
		rs.set<flag::h>(true);
		rs.set<flag::c>(false);
		break;
	case operation::or_:
		result = dst | src;
		rs.set<flag::z>(result == 0);
		rs.set<flag::n>(false);
		rs.set<flag::h>(false);
		rs.set<flag::c>(false);
		break;
	case operation::xor_:
		result = dst ^ src;
		rs.set<flag::z>(result == 0);
		rs.set<flag::n>(false);
		rs.set<flag::h>(false);
		rs.set<flag::c>(false);
		break;
	case operation::cp:
		execute_alu(operation::sub, dst, src, rs);
		result = dst;  // throw away subtraction result
		break;
	default:
		assert(false);
		return 0;
	}
	return result;
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
		assert(false);
		return {};
	}
}

bool check_condition(const gb::z80_cpu &cpu, cond c)
{
	auto &rs = cpu.registers();
	bool jump = false;

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
		assert(false);
		return false;
	}
}

// r = register
// i = intermediate
// m = memory denoted by address in register

template <gb::register8 Dst>
class opcode_ld_ri : public gb::opcode
{
public:
	opcode_ld_ri() : gb::opcode("LD " + to_string(Dst) + ",$", 1, 8) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write8<Dst>(cpu.value8());
	}
};

template <gb::register16 Dst>
class opcode_ld_mi : public gb::opcode
{
public:
	opcode_ld_mi() : gb::opcode("LD (" + to_string(Dst) + "),$", 1, 12) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.memory().write8(cpu.registers().read16<Dst>(), cpu.value8());
	}
};

template <gb::register8 Dst, gb::register8 Src>
class opcode_ld_rr : public gb::opcode
{
public:
	opcode_ld_rr() : gb::opcode("LD " + to_string(Dst) + "," + to_string(Src), 0, 4) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write8<Dst>(cpu.registers().read8<Src>());
	}
};

template <gb::register8 Dst, gb::register16 Src>
class opcode_ld_rm : public gb::opcode
{
public:
	opcode_ld_rm() : gb::opcode("LD " + to_string(Dst) + ",(" + to_string(Src) + ")", 0, 8) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write8<Dst>(cpu.memory().read8(cpu.registers().read16<Src>()));
	}
};

template <gb::register8 Dst>
class opcode_ld_rmi : public gb::opcode
{
public:
	opcode_ld_rmi() : gb::opcode("LD " + to_string(Dst) + ",($)", 2, 16) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write8<Dst>(cpu.memory().read8(cpu.value16()));
	}
};

template <gb::register8 Src>
class opcode_ld_mir : public gb::opcode
{
public:
	opcode_ld_mir() : gb::opcode("LD ($)," + to_string(Src), 2, 16)	{}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.memory().write8(cpu.value16(), cpu.registers().read8<Src>());
	}
};

template <gb::register16 Dst, gb::register8 Src>
class opcode_ld_mr : public gb::opcode
{
public:
	opcode_ld_mr() : gb::opcode("LD (" + to_string(Dst) + ")," + to_string(Src), 0, 8) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.memory().write8(cpu.registers().read16<Dst>(), cpu.registers().read8<Src>());
	}
};


class opcode_ldff_ac : public gb::opcode
{
public:
	opcode_ldff_ac() : gb::opcode("LD A,(ff00h+C)", 0, 8) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write8<r8::a>(cpu.memory().read8(0xFF00 + cpu.registers().read8<r8::c>()));
	}
};

class opcode_ldff_ai : public gb::opcode
{
public:
	opcode_ldff_ai() : gb::opcode("LD A,(ff00h+$)", 1, 8) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write8<r8::a>(cpu.memory().read8(0xFF00 + cpu.value8()));
	}
};

class opcode_ldff_ca : public gb::opcode
{
public:
	opcode_ldff_ca() : gb::opcode("LD (ff00h+C),A", 0, 8) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.memory().write8(0xFF00 + cpu.registers().read8<r8::c>(), cpu.registers().read8<r8::a>());
	}
};

class opcode_ldff_ia : public gb::opcode
{
public:
	opcode_ldff_ia() : gb::opcode("LD (ff00h+$),A", 1, 8) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.memory().write8(0xFF00 + cpu.value8(), cpu.registers().read8<r8::a>());
	}
};

template <bool Dec, bool AHL>
class opcode_lddi : public gb::opcode
{
public:
	opcode_lddi() :
		gb::opcode(std::string(Dec ? "LDD" : "LDI") + " " + (AHL ? "A,(HL)" : "(HL),A"), 0, 8)
	{}

	void execute(gb::z80_cpu &cpu) const override
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
	opcode_ld16_ri() : gb::opcode("LD " + to_string(Dst) + ",$", 2, 12) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write16<Dst>(cpu.value16());
	}
};

template <gb::register16 Dst, gb::register16 Src>
class opcode_ld16_rr : public gb::opcode
{
public:
	opcode_ld16_rr() : gb::opcode("LD " + to_string(Dst) + "," + to_string(Src), 0, 8) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write16<Dst>(cpu.registers().read16<Src>());
	}
};

class opcode_ld16_hlspn : public gb::opcode
{
public:
	opcode_ld16_hlspn() : gb::opcode("LD HL,SP+$", 1, 12) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		const uint16_t sp = cpu.registers().read16<r16::sp>();
		const uint16_t offset = sign_extend(cpu.value8());
		const uint16_t hl = sp + offset;

		cpu.registers().set<flag::z>(false);
		cpu.registers().set<flag::n>(false);
		cpu.registers().set<flag::h>((sp & 0x0FFF) > 0x0FFF - (offset & 0x0FFF));
		cpu.registers().set<flag::c>(sp > 0xFFFF - offset);
		cpu.registers().write16<r16::hl>(hl);
	}
};

template <gb::register16 Src>
class opcode_ld16_mir : public gb::opcode
{
public:
	opcode_ld16_mir() : gb::opcode("LD ($)," + to_string(Src), 2, 20) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.memory().write16(cpu.value16(), cpu.registers().read16<Src>());
	}
};


template <gb::register16 Src>
class opcode_push : public gb::opcode
{
public:
	opcode_push() : gb::opcode("PUSH " + to_string(Src), 0, 16) {}

	void execute(gb::z80_cpu &cpu) const override
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
	opcode_pop() : gb::opcode("POP " + to_string(Dst), 0, 12) {}

	void execute(gb::z80_cpu &cpu) const override
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
	opcode_alu_rr() : gb::opcode(to_string(Op) + " " + to_string(Dst) + "," + to_string(Src), 0, 4) {}
		
	void execute(gb::z80_cpu &cpu) const override
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
	opcode_alu_rm() : gb::opcode(to_string(Op) + " " + to_string(Dst) + ",(" + to_string(Src) + ")", 0, 8) {}
		
	void execute(gb::z80_cpu &cpu) const override
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
	opcode_alu_ri() : gb::opcode(to_string(Op) + " " + to_string(Dst) + ",$", 1, 8) {}
		
	void execute(gb::z80_cpu &cpu) const override
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
	opcode_decinc_r() : gb::opcode(std::string(Dec ? "DEC" : "INC") + " " + to_string(Dst), 0, 4) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		uint8_t value = cpu.registers().read8<Dst>();
		if (Dec)
		{
			cpu.registers().set<flag::n>(true);
			cpu.registers().set<flag::h>(value == 0x10);
			--value;
		}
		else  // Inc
		{
			cpu.registers().set<flag::n>(false);
			cpu.registers().set<flag::h>(value == 0x0F);
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
	opcode_decinc_rm() : gb::opcode(std::string(Dec ? "DEC" : "INC") + " (" + to_string(Dst) + ")", 0, 12) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		uint8_t value = cpu.memory().read8(cpu.registers().read16<Dst>());
		if (Dec)
		{
			cpu.registers().set<flag::n>(true);
			cpu.registers().set<flag::h>(value == 0x10);
			--value;
		}
		else  // Inc
		{
			cpu.registers().set<flag::n>(false);
			cpu.registers().set<flag::h>(value == 0x0F);
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
	opcode_add16_hl() : gb::opcode("ADD HL," + to_string(Src), 0, 8) {}

	void execute(gb::z80_cpu &cpu) const override
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
	opcode_add16_sp_i() : gb::opcode("ADD SP,$", 1, 16) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		uint16_t sp = cpu.registers().read16<r16::sp>();
		uint16_t offset = sign_extend(cpu.value8());

		cpu.registers().set<flag::z>(false);
		cpu.registers().set<flag::n>(false);
		cpu.registers().set<flag::h>((sp & 0x0FFF) > 0x0FFF - offset);
		cpu.registers().set<flag::c>(sp > 0xFFFF - offset);

		sp += offset;
		cpu.registers().write16<r16::sp>(sp);
	}
};

template <bool Dec, gb::register16 Dst>
class opcode_decinc16_r : public gb::opcode
{
public:
	opcode_decinc16_r() : gb::opcode(std::string(Dec ? "DEC" : "INC") + " " + to_string(Dst), 0, 8) {}

	void execute(gb::z80_cpu &cpu) const override
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
	opcode_daa() : gb::opcode("DAA", 0, 4) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		uint8_t value = cpu.registers().read8<r8::a>();
		if ((value & 0xF) > 9)
		{
			value += 6;
		}
		if (value > 0x9F || cpu.registers().get<flag::c>())
		{
			value += 0x60;
			cpu.registers().set<flag::c>(true);
		}
		cpu.registers().write8<r8::a>(value);
	}
};

class opcode_cpl : public gb::opcode
{
public:
	opcode_cpl() : gb::opcode("CPL", 0, 4) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write8<r8::a>(~cpu.registers().read8<r8::a>());
		cpu.registers().set<flag::n>(true);
		cpu.registers().set<flag::h>(true);
	}
};

class opcode_ccf : public gb::opcode
{
public:
	opcode_ccf() : gb::opcode("CCF", 0, 4) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().set<flag::n>(false);
		cpu.registers().set<flag::h>(false);
		cpu.registers().set<flag::c>(!cpu.registers().get<flag::c>());
	}
};

class opcode_scf : public gb::opcode
{
public:
	opcode_scf() : gb::opcode("SCF", 0, 4) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().set<flag::n>(false);
		cpu.registers().set<flag::h>(false);
		cpu.registers().set<flag::c>(true);
	}
};

class opcode_halt : public gb::opcode
{
public:
	opcode_halt() : gb::opcode("HALT", 0, 0) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.halt();
	}
};

class opcode_stop : public gb::opcode
{
public:
	opcode_stop() : gb::opcode("STOP", 0, 0) {}

	void execute(gb::z80_cpu &) const override
	{
		// TODO
		debug("WARNING: STOP nip");
	}
};

class opcode_di : public gb::opcode
{
public:
	opcode_di() : gb::opcode("DI", 0, 4) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.set_ime(false);
	}
};

class opcode_ei : public gb::opcode
{
public:
	opcode_ei() : gb::opcode("EI", 0, 4) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.set_ime(true);
	}
};

class opcode_nop : public gb::opcode
{
public:
	opcode_nop() : gb::opcode("NOP", 0, 4) {}
	void execute(gb::z80_cpu &) const override {}
};

template <bool Left, bool Carry> uint8_t rd_impl(gb::z80_cpu &cpu, uint8_t value)
{
	uint8_t bit;
	if (Left)  // <<<
	{
		bit = value & (1 << 7);
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
		bit = value & 1;
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

	cpu.registers().set<flag::z>(value == 0);
	cpu.registers().set<flag::n>(false);
	cpu.registers().set<flag::h>(false);

	return value;
}

template <bool Left, bool Carry>
class opcode_rda : public gb::opcode
{
public:
	opcode_rda() : gb::opcode(std::string("R") + (Left ? "L" : "R") + (Carry ? "C" : "") + "A", 0, 4) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write8<r8::a>(rd_impl<Left, Carry>(cpu, cpu.registers().read8<r8::a>()));
	}
};

template <cond Cond>
class opcode_jp_i : public gb::opcode
{
public:
	opcode_jp_i() : gb::opcode("JP " + to_string(Cond) + (Cond == cond::nop ? "" : ",") + "$", 2, 12) {}

	void execute(gb::z80_cpu &cpu) const override
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
	opcode_jp_hl() : gb::opcode("JP HL", 0, 4) {}

	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write16<r16::pc>(cpu.registers().read16<r16::hl>());
	}
};

template <cond Cond>
class opcode_jr_i : public gb::opcode
{
public:
	opcode_jr_i() : gb::opcode("JR " + to_string(Cond) + (Cond == cond::nop ? "" : ",") + "$", 1, 8) {}

	void execute(gb::z80_cpu &cpu) const override
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
	opcode_call() : gb::opcode("CALL " + to_string(Cond) + (Cond == cond::nop ? "" : ",") + "$", 2, 12) {}

	void execute(gb::z80_cpu &cpu) const override
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
	opcode_rst() : gb::opcode("RST " + std::to_string(Addr), 0, 32) {}

	void execute(gb::z80_cpu &cpu) const override
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
	opcode_ret() : gb::opcode(std::string("RET") + (Ei ? "I" : "") + (Cond == cond::nop ? std::string("") : (" " + to_string(Cond))), 0, 8) {}

	void execute(gb::z80_cpu &cpu) const override
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
	opcode_hang() : gb::opcode("HANG", 0, 0) {}
	
	void execute(gb::z80_cpu &) const override
	{
		debug("WARNING: Game used invalid opcode, ignoring (GB would hang)");
	}
};

gb::opcode_table init_opcodes()
{
	using r = gb::register8;
	using r16 = gb::register16;
	using std::make_unique;

	gb::opcode_table ops;
	ops[0x00] = make_unique<opcode_nop>();
	ops[0x01] = make_unique<opcode_ld16_ri<r16::bc>>();
	ops[0x02] = make_unique<opcode_ld_mr<r16::bc, r::a>>();
	ops[0x03] = make_unique<opcode_decinc16_r<false, r16::bc>>();
	ops[0x04] = make_unique<opcode_decinc_r<false, r::b>>();
	ops[0x05] = make_unique<opcode_decinc_r<true, r::b>>();
	ops[0x06] = make_unique<opcode_ld_ri<r::b>>();
	ops[0x07] = make_unique<opcode_rda<true, true>>();
	ops[0x08] = make_unique<opcode_ld16_mir<r16::sp>>();
	ops[0x09] = make_unique<opcode_add16_hl<r16::bc>>();
	ops[0x0A] = make_unique<opcode_ld_rm<r::a, r16::bc>>();
	ops[0x0B] = make_unique<opcode_decinc16_r<true, r16::bc>>();
	ops[0x0C] = make_unique<opcode_decinc_r<false, r::c>>();
	ops[0x0D] = make_unique<opcode_decinc_r<true, r::c>>();
	ops[0x0E] = make_unique<opcode_ld_ri<r::c>>();
	ops[0x0F] = make_unique<opcode_rda<false, true>>();
	ops[0x10] = make_unique<opcode_stop>();
	ops[0x11] = make_unique<opcode_ld16_ri<r16::de>>();
	ops[0x12] = make_unique<opcode_ld_mr<r16::de, r::a>>();
	ops[0x13] = make_unique<opcode_decinc16_r<false, r16::de>>();
	ops[0x14] = make_unique<opcode_decinc_r<false, r::d>>();
	ops[0x15] = make_unique<opcode_decinc_r<true, r::d>>();
	ops[0x16] = make_unique<opcode_ld_ri<r::d>>();
	ops[0x17] = make_unique<opcode_rda<true, false>>();
	ops[0x18] = make_unique<opcode_jr_i<cond::nop>>();
	ops[0x19] = make_unique<opcode_add16_hl<r16::de>>();
	ops[0x1A] = make_unique<opcode_ld_rm<r::a, r16::de>>();
	ops[0x1B] = make_unique<opcode_decinc16_r<true, r16::de>>();
	ops[0x1C] = make_unique<opcode_decinc_r<false, r::e>>();
	ops[0x1D] = make_unique<opcode_decinc_r<true, r::e>>();
	ops[0x1E] = make_unique<opcode_ld_ri<r::e>>();
	ops[0x1F] = make_unique<opcode_rda<false, false>>();
	ops[0x20] = make_unique<opcode_jr_i<cond::nz>>();
	ops[0x21] = make_unique<opcode_ld16_ri<r16::hl>>();
	ops[0x22] = make_unique<opcode_lddi<false, false>>();
	ops[0x23] = make_unique<opcode_decinc16_r<false, r16::hl>>();
	ops[0x24] = make_unique<opcode_decinc_r<false, r::h>>();
	ops[0x25] = make_unique<opcode_decinc_r<true, r::h>>();
	ops[0x26] = make_unique<opcode_ld_ri<r::h>>();
	ops[0x27] = make_unique<opcode_daa>();
	ops[0x28] = make_unique<opcode_jr_i<cond::z>>();
	ops[0x29] = make_unique<opcode_add16_hl<r16::hl>>();
	ops[0x2A] = make_unique<opcode_lddi<false, true>>();
	ops[0x2B] = make_unique<opcode_decinc16_r<true, r16::hl>>();
	ops[0x2C] = make_unique<opcode_decinc_r<false, r::l>>();
	ops[0x2D] = make_unique<opcode_decinc_r<true, r::l>>();
	ops[0x2E] = make_unique<opcode_ld_ri<r::l>>();
	ops[0x2F] = make_unique<opcode_cpl>();
	ops[0x30] = make_unique<opcode_jr_i<cond::nc>>();
	ops[0x31] = make_unique<opcode_ld16_ri<r16::sp>>();
	ops[0x32] = make_unique<opcode_lddi<true, false>>();
	ops[0x33] = make_unique<opcode_decinc16_r<false, r16::sp>>();
	ops[0x34] = make_unique<opcode_decinc_rm<false, r16::hl>>();
	ops[0x35] = make_unique<opcode_decinc_rm<true, r16::hl>>();
	ops[0x36] = make_unique<opcode_ld_mi<r16::hl>>();
	ops[0x37] = make_unique<opcode_scf>();
	ops[0x38] = make_unique<opcode_jr_i<cond::c>>();
	ops[0x39] = make_unique<opcode_add16_hl<r16::sp>>();
	ops[0x3A] = make_unique<opcode_lddi<true, true>>();
	ops[0x3B] = make_unique<opcode_decinc16_r<true, r16::sp>>();
	ops[0x3C] = make_unique<opcode_decinc_r<false, r::a>>();
	ops[0x3D] = make_unique<opcode_decinc_r<true, r::a>>();
	ops[0x3E] = make_unique<opcode_ld_ri<r::a>>();
	ops[0x3F] = make_unique<opcode_ccf>();
	ops[0x40] = make_unique<opcode_ld_rr<r::b, r::b>>();
	ops[0x41] = make_unique<opcode_ld_rr<r::b, r::c>>();
	ops[0x42] = make_unique<opcode_ld_rr<r::b, r::d>>();
	ops[0x43] = make_unique<opcode_ld_rr<r::b, r::e>>();
	ops[0x44] = make_unique<opcode_ld_rr<r::b, r::h>>();
	ops[0x45] = make_unique<opcode_ld_rr<r::b, r::l>>();
	ops[0x46] = make_unique<opcode_ld_rm<r::b, r16::hl>>();
	ops[0x47] = make_unique<opcode_ld_rr<r::b, r::a>>();
	ops[0x48] = make_unique<opcode_ld_rr<r::c, r::b>>();
	ops[0x49] = make_unique<opcode_ld_rr<r::c, r::c>>();
	ops[0x4A] = make_unique<opcode_ld_rr<r::c, r::d>>();
	ops[0x4B] = make_unique<opcode_ld_rr<r::c, r::e>>();
	ops[0x4C] = make_unique<opcode_ld_rr<r::c, r::h>>();
	ops[0x4D] = make_unique<opcode_ld_rr<r::c, r::l>>();
	ops[0x4E] = make_unique<opcode_ld_rm<r::c, r16::hl>>();
	ops[0x4F] = make_unique<opcode_ld_rr<r::c, r::a>>();
	ops[0x50] = make_unique<opcode_ld_rr<r::d, r::b>>();
	ops[0x51] = make_unique<opcode_ld_rr<r::d, r::c>>();
	ops[0x52] = make_unique<opcode_ld_rr<r::d, r::d>>();
	ops[0x53] = make_unique<opcode_ld_rr<r::d, r::e>>();
	ops[0x54] = make_unique<opcode_ld_rr<r::d, r::h>>();
	ops[0x55] = make_unique<opcode_ld_rr<r::d, r::l>>();
	ops[0x56] = make_unique<opcode_ld_rm<r::d, r16::hl>>();
	ops[0x57] = make_unique<opcode_ld_rr<r::d, r::a>>();
	ops[0x58] = make_unique<opcode_ld_rr<r::e, r::b>>();
	ops[0x59] = make_unique<opcode_ld_rr<r::e, r::c>>();
	ops[0x5A] = make_unique<opcode_ld_rr<r::e, r::d>>();
	ops[0x5B] = make_unique<opcode_ld_rr<r::e, r::e>>();
	ops[0x5C] = make_unique<opcode_ld_rr<r::e, r::h>>();
	ops[0x5D] = make_unique<opcode_ld_rr<r::e, r::l>>();
	ops[0x5E] = make_unique<opcode_ld_rm<r::e, r16::hl>>();
	ops[0x5F] = make_unique<opcode_ld_rr<r::e, r::a>>();
	ops[0x60] = make_unique<opcode_ld_rr<r::h, r::b>>();
	ops[0x61] = make_unique<opcode_ld_rr<r::h, r::c>>();
	ops[0x62] = make_unique<opcode_ld_rr<r::h, r::d>>();
	ops[0x63] = make_unique<opcode_ld_rr<r::h, r::e>>();
	ops[0x64] = make_unique<opcode_ld_rr<r::h, r::h>>();
	ops[0x65] = make_unique<opcode_ld_rr<r::h, r::l>>();
	ops[0x66] = make_unique<opcode_ld_rm<r::h, r16::hl>>();
	ops[0x67] = make_unique<opcode_ld_rr<r::h, r::a>>();
	ops[0x68] = make_unique<opcode_ld_rr<r::l, r::b>>();
	ops[0x69] = make_unique<opcode_ld_rr<r::l, r::c>>();
	ops[0x6A] = make_unique<opcode_ld_rr<r::l, r::d>>();
	ops[0x6B] = make_unique<opcode_ld_rr<r::l, r::e>>();
	ops[0x6C] = make_unique<opcode_ld_rr<r::l, r::h>>();
	ops[0x6D] = make_unique<opcode_ld_rr<r::l, r::l>>();
	ops[0x6E] = make_unique<opcode_ld_rm<r::l, r16::hl>>();
	ops[0x6F] = make_unique<opcode_ld_rr<r::l, r::a>>();
	ops[0x70] = make_unique<opcode_ld_mr<r16::hl, r::b>>();
	ops[0x71] = make_unique<opcode_ld_mr<r16::hl, r::c>>();
	ops[0x72] = make_unique<opcode_ld_mr<r16::hl, r::d>>();
	ops[0x73] = make_unique<opcode_ld_mr<r16::hl, r::e>>();
	ops[0x74] = make_unique<opcode_ld_mr<r16::hl, r::h>>();
	ops[0x75] = make_unique<opcode_ld_mr<r16::hl, r::l>>();
	ops[0x76] = make_unique<opcode_halt>();
	ops[0x77] = make_unique<opcode_ld_mr<r16::hl, r::a>>();
	ops[0x78] = make_unique<opcode_ld_rr<r::a, r::b>>();
	ops[0x79] = make_unique<opcode_ld_rr<r::a, r::c>>();
	ops[0x7A] = make_unique<opcode_ld_rr<r::a, r::d>>();
	ops[0x7B] = make_unique<opcode_ld_rr<r::a, r::e>>();
	ops[0x7C] = make_unique<opcode_ld_rr<r::a, r::h>>();
	ops[0x7D] = make_unique<opcode_ld_rr<r::a, r::l>>();
	ops[0x7E] = make_unique<opcode_ld_rm<r::a, r16::hl>>();
	ops[0x7F] = make_unique<opcode_ld_rr<r::a, r::a>>();
	ops[0x80] = make_unique<opcode_alu_rr<operation::add, r::a, r::b>>();
	ops[0x81] = make_unique<opcode_alu_rr<operation::add, r::a, r::c>>();
	ops[0x82] = make_unique<opcode_alu_rr<operation::add, r::a, r::d>>();
	ops[0x83] = make_unique<opcode_alu_rr<operation::add, r::a, r::e>>();
	ops[0x84] = make_unique<opcode_alu_rr<operation::add, r::a, r::h>>();
	ops[0x85] = make_unique<opcode_alu_rr<operation::add, r::a, r::l>>();
	ops[0x86] = make_unique<opcode_alu_rm<operation::add, r::a, r16::hl>>();
	ops[0x87] = make_unique<opcode_alu_rr<operation::add, r::a, r::a>>();
	ops[0x88] = make_unique<opcode_alu_rr<operation::adc, r::a, r::b>>();
	ops[0x89] = make_unique<opcode_alu_rr<operation::adc, r::a, r::c>>();
	ops[0x8A] = make_unique<opcode_alu_rr<operation::adc, r::a, r::d>>();
	ops[0x8B] = make_unique<opcode_alu_rr<operation::adc, r::a, r::e>>();
	ops[0x8C] = make_unique<opcode_alu_rr<operation::adc, r::a, r::h>>();
	ops[0x8D] = make_unique<opcode_alu_rr<operation::adc, r::a, r::l>>();
	ops[0x8E] = make_unique<opcode_alu_rm<operation::adc, r::a, r16::hl>>();
	ops[0x8F] = make_unique<opcode_alu_rr<operation::adc, r::a, r::a>>();
	ops[0x90] = make_unique<opcode_alu_rr<operation::sub, r::a, r::b>>();
	ops[0x91] = make_unique<opcode_alu_rr<operation::sub, r::a, r::c>>();
	ops[0x92] = make_unique<opcode_alu_rr<operation::sub, r::a, r::d>>();
	ops[0x93] = make_unique<opcode_alu_rr<operation::sub, r::a, r::e>>();
	ops[0x94] = make_unique<opcode_alu_rr<operation::sub, r::a, r::h>>();
	ops[0x95] = make_unique<opcode_alu_rr<operation::sub, r::a, r::l>>();
	ops[0x96] = make_unique<opcode_alu_rm<operation::sub, r::a, r16::hl>>();
	ops[0x97] = make_unique<opcode_alu_rr<operation::sub, r::a, r::a>>();
	ops[0x98] = make_unique<opcode_alu_rr<operation::sbc, r::a, r::b>>();
	ops[0x99] = make_unique<opcode_alu_rr<operation::sbc, r::a, r::c>>();
	ops[0x9A] = make_unique<opcode_alu_rr<operation::sbc, r::a, r::d>>();
	ops[0x9B] = make_unique<opcode_alu_rr<operation::sbc, r::a, r::e>>();
	ops[0x9C] = make_unique<opcode_alu_rr<operation::sbc, r::a, r::h>>();
	ops[0x9D] = make_unique<opcode_alu_rr<operation::sbc, r::a, r::l>>();
	ops[0x9E] = make_unique<opcode_alu_rm<operation::sbc, r::a, r16::hl>>();
	ops[0x9F] = make_unique<opcode_alu_rr<operation::sbc, r::a, r::a>>();
	ops[0xA0] = make_unique<opcode_alu_rr<operation::and_, r::a, r::b>>();
	ops[0xA1] = make_unique<opcode_alu_rr<operation::and_, r::a, r::c>>();
	ops[0xA2] = make_unique<opcode_alu_rr<operation::and_, r::a, r::d>>();
	ops[0xA3] = make_unique<opcode_alu_rr<operation::and_, r::a, r::e>>();
	ops[0xA4] = make_unique<opcode_alu_rr<operation::and_, r::a, r::h>>();
	ops[0xA5] = make_unique<opcode_alu_rr<operation::and_, r::a, r::l>>();
	ops[0xA6] = make_unique<opcode_alu_rm<operation::and_, r::a, r16::hl>>();
	ops[0xA7] = make_unique<opcode_alu_rr<operation::and_, r::a, r::a>>();
	ops[0xA8] = make_unique<opcode_alu_rr<operation::xor_, r::a, r::b>>();
	ops[0xA9] = make_unique<opcode_alu_rr<operation::xor_, r::a, r::c>>();
	ops[0xAA] = make_unique<opcode_alu_rr<operation::xor_, r::a, r::d>>();
	ops[0xAB] = make_unique<opcode_alu_rr<operation::xor_, r::a, r::e>>();
	ops[0xAC] = make_unique<opcode_alu_rr<operation::xor_, r::a, r::h>>();
	ops[0xAD] = make_unique<opcode_alu_rr<operation::xor_, r::a, r::l>>();
	ops[0xAE] = make_unique<opcode_alu_rm<operation::xor_, r::a, r16::hl>>();
	ops[0xAF] = make_unique<opcode_alu_rr<operation::xor_, r::a, r::a>>();
	ops[0xB0] = make_unique<opcode_alu_rr<operation::or_, r::a, r::b>>();
	ops[0xB1] = make_unique<opcode_alu_rr<operation::or_, r::a, r::c>>();
	ops[0xB2] = make_unique<opcode_alu_rr<operation::or_, r::a, r::d>>();
	ops[0xB3] = make_unique<opcode_alu_rr<operation::or_, r::a, r::e>>();
	ops[0xB4] = make_unique<opcode_alu_rr<operation::or_, r::a, r::h>>();
	ops[0xB5] = make_unique<opcode_alu_rr<operation::or_, r::a, r::l>>();
	ops[0xB6] = make_unique<opcode_alu_rm<operation::or_, r::a, r16::hl>>();
	ops[0xB7] = make_unique<opcode_alu_rr<operation::or_, r::a, r::a>>();
	ops[0xB8] = make_unique<opcode_alu_rr<operation::cp, r::a, r::b>>();
	ops[0xB9] = make_unique<opcode_alu_rr<operation::cp, r::a, r::c>>();
	ops[0xBA] = make_unique<opcode_alu_rr<operation::cp, r::a, r::d>>();
	ops[0xBB] = make_unique<opcode_alu_rr<operation::cp, r::a, r::e>>();
	ops[0xBC] = make_unique<opcode_alu_rr<operation::cp, r::a, r::h>>();
	ops[0xBD] = make_unique<opcode_alu_rr<operation::cp, r::a, r::l>>();
	ops[0xBE] = make_unique<opcode_alu_rm<operation::cp, r::a, r16::hl>>();
	ops[0xBF] = make_unique<opcode_alu_rr<operation::cp, r::a, r::a>>();
	ops[0xC0] = make_unique<opcode_ret<cond::nz, false>>();
	ops[0xC1] = make_unique<opcode_pop<r16::bc>>();
	ops[0xC2] = make_unique<opcode_jp_i<cond::nz>>();
	ops[0xC3] = make_unique<opcode_jp_i<cond::nop>>();
	ops[0xC4] = make_unique<opcode_call<cond::nz>>();
	ops[0xC5] = make_unique<opcode_push<r16::bc>>();
	ops[0xC6] = make_unique<opcode_alu_ri<operation::add, r::a>>();
	ops[0xC7] = make_unique<opcode_rst<0x00>>();
	ops[0xC8] = make_unique<opcode_ret<cond::z, false>>();
	ops[0xC9] = make_unique<opcode_ret<cond::nop, false>>();
	ops[0xCA] = make_unique<opcode_jp_i<cond::z>>();
	ops[0xCB] = nullptr;
	ops[0xCC] = make_unique<opcode_call<cond::z>>();
	ops[0xCD] = make_unique<opcode_call<cond::nop>>();
	ops[0xCE] = make_unique<opcode_alu_ri<operation::adc, r::a>>();
	ops[0xCF] = make_unique<opcode_rst<0x08>>();
	ops[0xD0] = make_unique<opcode_ret<cond::nc, false>>();
	ops[0xD1] = make_unique<opcode_pop<r16::de>>();
	ops[0xD2] = make_unique<opcode_jp_i<cond::nc>>();
	ops[0xD3] = nullptr;
	ops[0xD4] = make_unique<opcode_call<cond::nc>>();
	ops[0xD5] = make_unique<opcode_push<r16::de>>();
	ops[0xD6] = make_unique<opcode_alu_ri<operation::sub, r::a>>();
	ops[0xD7] = make_unique<opcode_rst<0x10>>();
	ops[0xD8] = make_unique<opcode_ret<cond::c, false>>();
	ops[0xD9] = make_unique<opcode_ret<cond::nop, true>>();
	ops[0xDA] = make_unique<opcode_jp_i<cond::c>>();
	ops[0xDB] = nullptr;
	ops[0xDC] = make_unique<opcode_call<cond::c>>();
	ops[0xDD] = nullptr;
	ops[0xDE] = make_unique<opcode_alu_ri<operation::sbc, r::a>>();
	ops[0xDF] = make_unique<opcode_rst<0x18>>();
	ops[0xE0] = make_unique<opcode_ldff_ia>();
	ops[0xE1] = make_unique<opcode_pop<r16::hl>>();
	ops[0xE2] = make_unique<opcode_ldff_ca>();
	ops[0xE3] = nullptr;
	ops[0xE4] = nullptr;
	ops[0xE5] = make_unique<opcode_push<r16::hl>>();
	ops[0xE6] = make_unique<opcode_alu_ri<operation::and_, r::a>>();
	ops[0xE7] = make_unique<opcode_rst<0x20>>();
	ops[0xE8] = make_unique<opcode_add16_sp_i>();
	ops[0xE9] = make_unique<opcode_jp_hl>();
	ops[0xEA] = make_unique<opcode_ld_mir<r::a>>();
	ops[0xEB] = nullptr;
	ops[0xEC] = nullptr;
	ops[0xED] = nullptr;
	ops[0xEE] = make_unique<opcode_alu_ri<operation::xor_, r::a>>();
	ops[0xEF] = make_unique<opcode_rst<0x28>>();
	ops[0xF0] = make_unique<opcode_ldff_ai>();
	ops[0xF1] = make_unique<opcode_pop<r16::af>>();
	ops[0xF2] = make_unique<opcode_ldff_ac>();
	ops[0xF3] = make_unique<opcode_di>();
	ops[0xF4] = nullptr;
	ops[0xF5] = make_unique<opcode_push<r16::af>>();
	ops[0xF6] = make_unique<opcode_alu_ri<operation::or_, r::a>>();
	ops[0xF7] = make_unique<opcode_rst<0x30>>();
	ops[0xF8] = make_unique<opcode_ld16_hlspn>();
	ops[0xF9] = make_unique<opcode_ld16_rr<r16::sp, r16::hl>>();
	ops[0xFA] = make_unique<opcode_ld_rmi<r::a>>();
	ops[0xFB] = make_unique<opcode_ei>();
	ops[0xFC] = nullptr;
	ops[0xFD] = nullptr;
	ops[0xFE] = make_unique<opcode_alu_ri<operation::cp, r::a>>();
	ops[0xFF] = make_unique<opcode_rst<0x38>>();

	for (auto &op : ops)
		if (op == nullptr)
			op = make_unique<opcode_hang>();

	return ops;
}

template <bool Left, bool Carry, gb::register8 Dst>
class opcode_cb_rdc_r : public gb::opcode
{
public:
	opcode_cb_rdc_r() :
		gb::opcode(std::string("R") + (Left ? "L" : "R") + (Carry ? "C" : "") + " " + to_string(Dst), 0, 8)
	{}
	
	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write8<Dst>(rd_impl<Left, Carry>(cpu, cpu.registers().read8<Dst>()));
	}
};

template <bool Left, bool Carry, gb::register16 Dst>
class opcode_cb_rdc_m : public gb::opcode
{
public:
	opcode_cb_rdc_m() :
		gb::opcode(std::string("R") + (Left ? "L" : "R") + (Carry ? "C" : "") + " (" + to_string(Dst) + ")", 0, 16)
	{}
	
	void execute(gb::z80_cpu &cpu) const override
	{
		uint16_t addr = cpu.registers().read16<Dst>();
		cpu.memory().write8(addr, rd_impl<Left, Carry>(cpu, cpu.memory().read8(addr)));
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
		gb::opcode(std::string("S") + (Left ? "L" : "R") + "A " + to_string(Dst), 0, 8)
	{}
	
	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write8<Dst>(sda_impl<Left>(cpu, cpu.registers().read8<Dst>()));
	}
};

template <bool Left, gb::register16 Dst>
class opcode_cb_sda_m : public gb::opcode
{
public:
	opcode_cb_sda_m() :
		gb::opcode(std::string("S") + (Left ? "L" : "R") + "A (" + to_string(Dst) + ")", 0, 16)
	{}
	
	void execute(gb::z80_cpu &cpu) const override
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
		gb::opcode("SWAP " + to_string(Dst), 0, 8)
	{}
	
	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write8<Dst>(swap_impl(cpu, cpu.registers().read8<Dst>()));
	}
};

template <gb::register16 Dst>
class opcode_cb_swap_m : public gb::opcode
{
public:
	opcode_cb_swap_m() :
		gb::opcode("SWAP (" + to_string(Dst) + ")", 0, 16)
	{}
	
	void execute(gb::z80_cpu &cpu) const override
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
		gb::opcode("SRL " + to_string(Dst), 0, 8)
	{}
	
	void execute(gb::z80_cpu &cpu) const override
	{
		cpu.registers().write8<Dst>(srl_impl(cpu, cpu.registers().read8<Dst>()));
	}
};

template <gb::register16 Dst>
class opcode_cb_srl_m : public gb::opcode
{
public:
	opcode_cb_srl_m() :
		gb::opcode("SRL (" + to_string(Dst) + ")", 0, 16)
	{}
	
	void execute(gb::z80_cpu &cpu) const override
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
		gb::opcode("BIT " + std::to_string(bit) + "," + to_string(Dst), 0, 8)
	{}
	
	void execute(gb::z80_cpu &cpu) const override
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
		gb::opcode("BIT " + std::to_string(bit) + ",(" + to_string(Dst) + ")", 0, 16)
	{}
	
	void execute(gb::z80_cpu &cpu) const override
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
		gb::opcode((res ? "RES " : "SET ") + std::to_string(bit) + "," + to_string(Dst), 0, 8)
	{}
	
	void execute(gb::z80_cpu &cpu) const override
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
		gb::opcode((res ? "RES " : "SET ") + std::to_string(bit) + ",(" + to_string(Dst) + ")", 0, 16)
	{}
	
	void execute(gb::z80_cpu &cpu) const override
	{
		uint8_t b = 1 << bit;
		uint16_t addr = cpu.registers().read16<Dst>();
		if (res)
			cpu.memory().write8(addr, cpu.memory().read8(addr) & ~b);
		else  // set
			cpu.memory().write8(addr, cpu.memory().read8(addr) | b);
	}
};

gb::opcode_table init_cb_opcodes()
{
	using r = gb::register8;
	using r16 = gb::register16;
	using std::make_unique;

	gb::opcode_table ops;
	ops[0x00] = make_unique<opcode_cb_rdc_r<true, true, r::b>>();
	ops[0x01] = make_unique<opcode_cb_rdc_r<true, true, r::c>>();
	ops[0x02] = make_unique<opcode_cb_rdc_r<true, true, r::d>>();
	ops[0x03] = make_unique<opcode_cb_rdc_r<true, true, r::e>>();
	ops[0x04] = make_unique<opcode_cb_rdc_r<true, true, r::h>>();
	ops[0x05] = make_unique<opcode_cb_rdc_r<true, true, r::l>>();
	ops[0x06] = make_unique<opcode_cb_rdc_m<true, true, r16::hl>>();
	ops[0x07] = make_unique<opcode_cb_rdc_r<true, true, r::a>>();
	ops[0x08] = make_unique<opcode_cb_rdc_r<false, true, r::b>>();
	ops[0x09] = make_unique<opcode_cb_rdc_r<false, true, r::c>>();
	ops[0x0A] = make_unique<opcode_cb_rdc_r<false, true, r::d>>();
	ops[0x0B] = make_unique<opcode_cb_rdc_r<false, true, r::e>>();
	ops[0x0C] = make_unique<opcode_cb_rdc_r<false, true, r::h>>();
	ops[0x0D] = make_unique<opcode_cb_rdc_r<false, true, r::l>>();
	ops[0x0E] = make_unique<opcode_cb_rdc_m<false, true, r16::hl>>();
	ops[0x0F] = make_unique<opcode_cb_rdc_r<false, true, r::l>>();
	ops[0x10] = make_unique<opcode_cb_rdc_r<true, false, r::b>>();
	ops[0x11] = make_unique<opcode_cb_rdc_r<true, false, r::c>>();
	ops[0x12] = make_unique<opcode_cb_rdc_r<true, false, r::d>>();
	ops[0x13] = make_unique<opcode_cb_rdc_r<true, false, r::e>>();
	ops[0x14] = make_unique<opcode_cb_rdc_r<true, false, r::h>>();
	ops[0x15] = make_unique<opcode_cb_rdc_r<true, false, r::l>>();
	ops[0x16] = make_unique<opcode_cb_rdc_m<true, false, r16::hl>>();
	ops[0x17] = make_unique<opcode_cb_rdc_r<true, false, r::a>>();
	ops[0x18] = make_unique<opcode_cb_rdc_r<false, false, r::b>>();
	ops[0x19] = make_unique<opcode_cb_rdc_r<false, false, r::c>>();
	ops[0x1A] = make_unique<opcode_cb_rdc_r<false, false, r::d>>();
	ops[0x1B] = make_unique<opcode_cb_rdc_r<false, false, r::e>>();
	ops[0x1C] = make_unique<opcode_cb_rdc_r<false, false, r::h>>();
	ops[0x1D] = make_unique<opcode_cb_rdc_r<false, false, r::l>>();
	ops[0x1E] = make_unique<opcode_cb_rdc_m<false, false, r16::hl>>();
	ops[0x1F] = make_unique<opcode_cb_rdc_r<false, false, r::l>>();
	ops[0x20] = make_unique<opcode_cb_sda_r<true, r::b>>();
	ops[0x21] = make_unique<opcode_cb_sda_r<true, r::c>>();
	ops[0x22] = make_unique<opcode_cb_sda_r<true, r::d>>();
	ops[0x23] = make_unique<opcode_cb_sda_r<true, r::e>>();
	ops[0x24] = make_unique<opcode_cb_sda_r<true, r::h>>();
	ops[0x25] = make_unique<opcode_cb_sda_r<true, r::l>>();
	ops[0x26] = make_unique<opcode_cb_sda_m<true, r16::hl>>();
	ops[0x27] = make_unique<opcode_cb_sda_r<true, r::a>>();
	ops[0x28] = make_unique<opcode_cb_sda_r<false, r::b>>();
	ops[0x29] = make_unique<opcode_cb_sda_r<false, r::c>>();
	ops[0x2A] = make_unique<opcode_cb_sda_r<false, r::d>>();
	ops[0x2B] = make_unique<opcode_cb_sda_r<false, r::e>>();
	ops[0x2C] = make_unique<opcode_cb_sda_r<false, r::h>>();
	ops[0x2D] = make_unique<opcode_cb_sda_r<false, r::l>>();
	ops[0x2E] = make_unique<opcode_cb_sda_m<false, r16::hl>>();
	ops[0x2F] = make_unique<opcode_cb_sda_r<false, r::a>>();
	ops[0x30] = make_unique<opcode_cb_swap_r<r::b>>();
	ops[0x31] = make_unique<opcode_cb_swap_r<r::c>>();
	ops[0x32] = make_unique<opcode_cb_swap_r<r::d>>();
	ops[0x33] = make_unique<opcode_cb_swap_r<r::e>>();
	ops[0x34] = make_unique<opcode_cb_swap_r<r::h>>();
	ops[0x35] = make_unique<opcode_cb_swap_r<r::l>>();
	ops[0x36] = make_unique<opcode_cb_swap_m<r16::hl>>();
	ops[0x37] = make_unique<opcode_cb_swap_r<r::a>>();
	ops[0x38] = make_unique<opcode_cb_srl_r<r::b>>();
	ops[0x39] = make_unique<opcode_cb_srl_r<r::c>>();
	ops[0x3A] = make_unique<opcode_cb_srl_r<r::d>>();
	ops[0x3B] = make_unique<opcode_cb_srl_r<r::e>>();
	ops[0x3C] = make_unique<opcode_cb_srl_r<r::h>>();
	ops[0x3D] = make_unique<opcode_cb_srl_r<r::l>>();
	ops[0x3E] = make_unique<opcode_cb_srl_m<r16::hl>>();
	ops[0x3F] = make_unique<opcode_cb_srl_r<r::a>>();
	ops[0x40] = make_unique<opcode_cb_bit_r<0, r::b>>();
	ops[0x41] = make_unique<opcode_cb_bit_r<0, r::c>>();
	ops[0x42] = make_unique<opcode_cb_bit_r<0, r::d>>();
	ops[0x43] = make_unique<opcode_cb_bit_r<0, r::e>>();
	ops[0x44] = make_unique<opcode_cb_bit_r<0, r::h>>();
	ops[0x45] = make_unique<opcode_cb_bit_r<0, r::l>>();
	ops[0x46] = make_unique<opcode_cb_bit_m<0, r16::hl>>();
	ops[0x47] = make_unique<opcode_cb_bit_r<0, r::a>>();
	ops[0x48] = make_unique<opcode_cb_bit_r<1, r::b>>();
	ops[0x49] = make_unique<opcode_cb_bit_r<1, r::c>>();
	ops[0x4A] = make_unique<opcode_cb_bit_r<1, r::d>>();
	ops[0x4B] = make_unique<opcode_cb_bit_r<1, r::e>>();
	ops[0x4C] = make_unique<opcode_cb_bit_r<1, r::h>>();
	ops[0x4D] = make_unique<opcode_cb_bit_r<1, r::l>>();
	ops[0x4E] = make_unique<opcode_cb_bit_m<1, r16::hl>>();
	ops[0x4F] = make_unique<opcode_cb_bit_r<1, r::a>>();
	ops[0x50] = make_unique<opcode_cb_bit_r<2, r::b>>();
	ops[0x51] = make_unique<opcode_cb_bit_r<2, r::c>>();
	ops[0x52] = make_unique<opcode_cb_bit_r<2, r::d>>();
	ops[0x53] = make_unique<opcode_cb_bit_r<2, r::e>>();
	ops[0x54] = make_unique<opcode_cb_bit_r<2, r::h>>();
	ops[0x55] = make_unique<opcode_cb_bit_r<2, r::l>>();
	ops[0x56] = make_unique<opcode_cb_bit_m<2, r16::hl>>();
	ops[0x57] = make_unique<opcode_cb_bit_r<2, r::a>>();
	ops[0x58] = make_unique<opcode_cb_bit_r<3, r::b>>();
	ops[0x59] = make_unique<opcode_cb_bit_r<3, r::c>>();
	ops[0x5A] = make_unique<opcode_cb_bit_r<3, r::d>>();
	ops[0x5B] = make_unique<opcode_cb_bit_r<3, r::e>>();
	ops[0x5C] = make_unique<opcode_cb_bit_r<3, r::h>>();
	ops[0x5D] = make_unique<opcode_cb_bit_r<3, r::l>>();
	ops[0x5E] = make_unique<opcode_cb_bit_m<3, r16::hl>>();
	ops[0x5F] = make_unique<opcode_cb_bit_r<3, r::a>>();
	ops[0x60] = make_unique<opcode_cb_bit_r<4, r::b>>();
	ops[0x61] = make_unique<opcode_cb_bit_r<4, r::c>>();
	ops[0x62] = make_unique<opcode_cb_bit_r<4, r::d>>();
	ops[0x63] = make_unique<opcode_cb_bit_r<4, r::e>>();
	ops[0x64] = make_unique<opcode_cb_bit_r<4, r::h>>();
	ops[0x65] = make_unique<opcode_cb_bit_r<4, r::l>>();
	ops[0x66] = make_unique<opcode_cb_bit_m<4, r16::hl>>();
	ops[0x67] = make_unique<opcode_cb_bit_r<4, r::a>>();
	ops[0x68] = make_unique<opcode_cb_bit_r<5, r::b>>();
	ops[0x69] = make_unique<opcode_cb_bit_r<5, r::c>>();
	ops[0x6A] = make_unique<opcode_cb_bit_r<5, r::d>>();
	ops[0x6B] = make_unique<opcode_cb_bit_r<5, r::e>>();
	ops[0x6C] = make_unique<opcode_cb_bit_r<5, r::h>>();
	ops[0x6D] = make_unique<opcode_cb_bit_r<5, r::l>>();
	ops[0x6E] = make_unique<opcode_cb_bit_m<5, r16::hl>>();
	ops[0x6F] = make_unique<opcode_cb_bit_r<5, r::a>>();
	ops[0x70] = make_unique<opcode_cb_bit_r<6, r::b>>();
	ops[0x71] = make_unique<opcode_cb_bit_r<6, r::c>>();
	ops[0x72] = make_unique<opcode_cb_bit_r<6, r::d>>();
	ops[0x73] = make_unique<opcode_cb_bit_r<6, r::e>>();
	ops[0x74] = make_unique<opcode_cb_bit_r<6, r::h>>();
	ops[0x75] = make_unique<opcode_cb_bit_r<6, r::l>>();
	ops[0x76] = make_unique<opcode_cb_bit_m<6, r16::hl>>();
	ops[0x77] = make_unique<opcode_cb_bit_r<6, r::a>>();
	ops[0x78] = make_unique<opcode_cb_bit_r<7, r::b>>();
	ops[0x79] = make_unique<opcode_cb_bit_r<7, r::c>>();
	ops[0x7A] = make_unique<opcode_cb_bit_r<7, r::d>>();
	ops[0x7B] = make_unique<opcode_cb_bit_r<7, r::e>>();
	ops[0x7C] = make_unique<opcode_cb_bit_r<7, r::h>>();
	ops[0x7D] = make_unique<opcode_cb_bit_r<7, r::l>>();
	ops[0x7E] = make_unique<opcode_cb_bit_m<7, r16::hl>>();
	ops[0x7F] = make_unique<opcode_cb_bit_r<7, r::a>>();
	ops[0x80] = make_unique<opcode_cb_resset_r<true, 0, r::b>>();
	ops[0x81] = make_unique<opcode_cb_resset_r<true, 0, r::c>>();
	ops[0x82] = make_unique<opcode_cb_resset_r<true, 0, r::d>>();
	ops[0x83] = make_unique<opcode_cb_resset_r<true, 0, r::e>>();
	ops[0x84] = make_unique<opcode_cb_resset_r<true, 0, r::h>>();
	ops[0x85] = make_unique<opcode_cb_resset_r<true, 0, r::l>>();
	ops[0x86] = make_unique<opcode_cb_resset_m<true, 0, r16::hl>>();
	ops[0x87] = make_unique<opcode_cb_resset_r<true, 0, r::a>>();
	ops[0x88] = make_unique<opcode_cb_resset_r<true, 1, r::b>>();
	ops[0x89] = make_unique<opcode_cb_resset_r<true, 1, r::c>>();
	ops[0x8A] = make_unique<opcode_cb_resset_r<true, 1, r::d>>();
	ops[0x8B] = make_unique<opcode_cb_resset_r<true, 1, r::e>>();
	ops[0x8C] = make_unique<opcode_cb_resset_r<true, 1, r::h>>();
	ops[0x8D] = make_unique<opcode_cb_resset_r<true, 1, r::l>>();
	ops[0x8E] = make_unique<opcode_cb_resset_m<true, 1, r16::hl>>();
	ops[0x8F] = make_unique<opcode_cb_resset_r<true, 1, r::a>>();
	ops[0x90] = make_unique<opcode_cb_resset_r<true, 2, r::b>>();
	ops[0x91] = make_unique<opcode_cb_resset_r<true, 2, r::c>>();
	ops[0x92] = make_unique<opcode_cb_resset_r<true, 2, r::d>>();
	ops[0x93] = make_unique<opcode_cb_resset_r<true, 2, r::e>>();
	ops[0x94] = make_unique<opcode_cb_resset_r<true, 2, r::h>>();
	ops[0x95] = make_unique<opcode_cb_resset_r<true, 2, r::l>>();
	ops[0x96] = make_unique<opcode_cb_resset_m<true, 2, r16::hl>>();
	ops[0x97] = make_unique<opcode_cb_resset_r<true, 2, r::a>>();
	ops[0x98] = make_unique<opcode_cb_resset_r<true, 3, r::b>>();
	ops[0x99] = make_unique<opcode_cb_resset_r<true, 3, r::c>>();
	ops[0x9A] = make_unique<opcode_cb_resset_r<true, 3, r::d>>();
	ops[0x9B] = make_unique<opcode_cb_resset_r<true, 3, r::e>>();
	ops[0x9C] = make_unique<opcode_cb_resset_r<true, 3, r::h>>();
	ops[0x9D] = make_unique<opcode_cb_resset_r<true, 3, r::l>>();
	ops[0x9E] = make_unique<opcode_cb_resset_m<true, 3, r16::hl>>();
	ops[0x9F] = make_unique<opcode_cb_resset_r<true, 3, r::a>>();
	ops[0xA0] = make_unique<opcode_cb_resset_r<true, 4, r::b>>();
	ops[0xA1] = make_unique<opcode_cb_resset_r<true, 4, r::c>>();
	ops[0xA2] = make_unique<opcode_cb_resset_r<true, 4, r::d>>();
	ops[0xA3] = make_unique<opcode_cb_resset_r<true, 4, r::e>>();
	ops[0xA4] = make_unique<opcode_cb_resset_r<true, 4, r::h>>();
	ops[0xA5] = make_unique<opcode_cb_resset_r<true, 4, r::l>>();
	ops[0xA6] = make_unique<opcode_cb_resset_m<true, 4, r16::hl>>();
	ops[0xA7] = make_unique<opcode_cb_resset_r<true, 4, r::a>>();
	ops[0xA8] = make_unique<opcode_cb_resset_r<true, 5, r::b>>();
	ops[0xA9] = make_unique<opcode_cb_resset_r<true, 5, r::c>>();
	ops[0xAA] = make_unique<opcode_cb_resset_r<true, 5, r::d>>();
	ops[0xAB] = make_unique<opcode_cb_resset_r<true, 5, r::e>>();
	ops[0xAC] = make_unique<opcode_cb_resset_r<true, 5, r::h>>();
	ops[0xAD] = make_unique<opcode_cb_resset_r<true, 5, r::l>>();
	ops[0xAE] = make_unique<opcode_cb_resset_m<true, 5, r16::hl>>();
	ops[0xAF] = make_unique<opcode_cb_resset_r<true, 5, r::a>>();
	ops[0xB0] = make_unique<opcode_cb_resset_r<true, 6, r::b>>();
	ops[0xB1] = make_unique<opcode_cb_resset_r<true, 6, r::c>>();
	ops[0xB2] = make_unique<opcode_cb_resset_r<true, 6, r::d>>();
	ops[0xB3] = make_unique<opcode_cb_resset_r<true, 6, r::e>>();
	ops[0xB4] = make_unique<opcode_cb_resset_r<true, 6, r::h>>();
	ops[0xB5] = make_unique<opcode_cb_resset_r<true, 6, r::l>>();
	ops[0xB6] = make_unique<opcode_cb_resset_m<true, 6, r16::hl>>();
	ops[0xB7] = make_unique<opcode_cb_resset_r<true, 6, r::a>>();
	ops[0xB8] = make_unique<opcode_cb_resset_r<true, 7, r::b>>();
	ops[0xB9] = make_unique<opcode_cb_resset_r<true, 7, r::c>>();
	ops[0xBA] = make_unique<opcode_cb_resset_r<true, 7, r::d>>();
	ops[0xBB] = make_unique<opcode_cb_resset_r<true, 7, r::e>>();
	ops[0xBC] = make_unique<opcode_cb_resset_r<true, 7, r::h>>();
	ops[0xBD] = make_unique<opcode_cb_resset_r<true, 7, r::l>>();
	ops[0xBE] = make_unique<opcode_cb_resset_m<true, 7, r16::hl>>();
	ops[0xBF] = make_unique<opcode_cb_resset_r<true, 7, r::a>>();
	ops[0xC0] = make_unique<opcode_cb_resset_r<false, 0, r::b>>();
	ops[0xC1] = make_unique<opcode_cb_resset_r<false, 0, r::c>>();
	ops[0xC2] = make_unique<opcode_cb_resset_r<false, 0, r::d>>();
	ops[0xC3] = make_unique<opcode_cb_resset_r<false, 0, r::e>>();
	ops[0xC4] = make_unique<opcode_cb_resset_r<false, 0, r::h>>();
	ops[0xC5] = make_unique<opcode_cb_resset_r<false, 0, r::l>>();
	ops[0xC6] = make_unique<opcode_cb_resset_m<false, 0, r16::hl>>();
	ops[0xC7] = make_unique<opcode_cb_resset_r<false, 0, r::a>>();
	ops[0xC8] = make_unique<opcode_cb_resset_r<false, 1, r::b>>();
	ops[0xC9] = make_unique<opcode_cb_resset_r<false, 1, r::c>>();
	ops[0xCA] = make_unique<opcode_cb_resset_r<false, 1, r::d>>();
	ops[0xCB] = make_unique<opcode_cb_resset_r<false, 1, r::e>>();
	ops[0xCC] = make_unique<opcode_cb_resset_r<false, 1, r::h>>();
	ops[0xCD] = make_unique<opcode_cb_resset_r<false, 1, r::l>>();
	ops[0xCE] = make_unique<opcode_cb_resset_m<false, 1, r16::hl>>();
	ops[0xCF] = make_unique<opcode_cb_resset_r<false, 1, r::a>>();
	ops[0xD0] = make_unique<opcode_cb_resset_r<false, 2, r::b>>();
	ops[0xD1] = make_unique<opcode_cb_resset_r<false, 2, r::c>>();
	ops[0xD2] = make_unique<opcode_cb_resset_r<false, 2, r::d>>();
	ops[0xD3] = make_unique<opcode_cb_resset_r<false, 2, r::e>>();
	ops[0xD4] = make_unique<opcode_cb_resset_r<false, 2, r::h>>();
	ops[0xD5] = make_unique<opcode_cb_resset_r<false, 2, r::l>>();
	ops[0xD6] = make_unique<opcode_cb_resset_m<false, 2, r16::hl>>();
	ops[0xD7] = make_unique<opcode_cb_resset_r<false, 2, r::a>>();
	ops[0xD8] = make_unique<opcode_cb_resset_r<false, 3, r::b>>();
	ops[0xD9] = make_unique<opcode_cb_resset_r<false, 3, r::c>>();
	ops[0xDA] = make_unique<opcode_cb_resset_r<false, 3, r::d>>();
	ops[0xDB] = make_unique<opcode_cb_resset_r<false, 3, r::e>>();
	ops[0xDC] = make_unique<opcode_cb_resset_r<false, 3, r::h>>();
	ops[0xDD] = make_unique<opcode_cb_resset_r<false, 3, r::l>>();
	ops[0xDE] = make_unique<opcode_cb_resset_m<false, 3, r16::hl>>();
	ops[0xDF] = make_unique<opcode_cb_resset_r<false, 3, r::a>>();
	ops[0xE0] = make_unique<opcode_cb_resset_r<false, 4, r::b>>();
	ops[0xE1] = make_unique<opcode_cb_resset_r<false, 4, r::c>>();
	ops[0xE2] = make_unique<opcode_cb_resset_r<false, 4, r::d>>();
	ops[0xE3] = make_unique<opcode_cb_resset_r<false, 4, r::e>>();
	ops[0xE4] = make_unique<opcode_cb_resset_r<false, 4, r::h>>();
	ops[0xE5] = make_unique<opcode_cb_resset_r<false, 4, r::l>>();
	ops[0xE6] = make_unique<opcode_cb_resset_m<false, 4, r16::hl>>();
	ops[0xE7] = make_unique<opcode_cb_resset_r<false, 4, r::a>>();
	ops[0xE8] = make_unique<opcode_cb_resset_r<false, 5, r::b>>();
	ops[0xE9] = make_unique<opcode_cb_resset_r<false, 5, r::c>>();
	ops[0xEA] = make_unique<opcode_cb_resset_r<false, 5, r::d>>();
	ops[0xEB] = make_unique<opcode_cb_resset_r<false, 5, r::e>>();
	ops[0xEC] = make_unique<opcode_cb_resset_r<false, 5, r::h>>();
	ops[0xED] = make_unique<opcode_cb_resset_r<false, 5, r::l>>();
	ops[0xEE] = make_unique<opcode_cb_resset_m<false, 5, r16::hl>>();
	ops[0xEF] = make_unique<opcode_cb_resset_r<false, 5, r::a>>();
	ops[0xF0] = make_unique<opcode_cb_resset_r<false, 6, r::b>>();
	ops[0xF1] = make_unique<opcode_cb_resset_r<false, 6, r::c>>();
	ops[0xF2] = make_unique<opcode_cb_resset_r<false, 6, r::d>>();
	ops[0xF3] = make_unique<opcode_cb_resset_r<false, 6, r::e>>();
	ops[0xF4] = make_unique<opcode_cb_resset_r<false, 6, r::h>>();
	ops[0xF5] = make_unique<opcode_cb_resset_r<false, 6, r::l>>();
	ops[0xF6] = make_unique<opcode_cb_resset_m<false, 6, r16::hl>>();
	ops[0xF7] = make_unique<opcode_cb_resset_r<false, 6, r::a>>();
	ops[0xF8] = make_unique<opcode_cb_resset_r<false, 7, r::b>>();
	ops[0xF9] = make_unique<opcode_cb_resset_r<false, 7, r::c>>();
	ops[0xFA] = make_unique<opcode_cb_resset_r<false, 7, r::d>>();
	ops[0xFB] = make_unique<opcode_cb_resset_r<false, 7, r::e>>();
	ops[0xFC] = make_unique<opcode_cb_resset_r<false, 7, r::h>>();
	ops[0xFD] = make_unique<opcode_cb_resset_r<false, 7, r::l>>();
	ops[0xFE] = make_unique<opcode_cb_resset_m<false, 7, r16::hl>>();
	ops[0xFF] = make_unique<opcode_cb_resset_r<false, 7, r::a>>();

	return ops;
}

}

gb::opcode::opcode(std::string mnemonic, int extra_bytes, int cycles) :
	_mnemonic(std::move(mnemonic)),
	_extra_bytes(extra_bytes),
	_cycles(cycles)
{
}

const gb::opcode_table gb::opcodes(init_opcodes());
const gb::opcode_table gb::cb_opcodes(init_cb_opcodes());
