#pragma once
#include "assert.hpp"
#include <string>
#include <array>

namespace gb
{
class z80_cpu;

class opcode
{
	using code = void (*)(z80_cpu &cpu);

public:
	opcode(std::string mnemonic, int extra_bytes, int cycles, code code, int extra_cycles=0) :
		_extra_bytes(extra_bytes),
		_cycles(cycles),
		_extra_cycles(extra_cycles),
		_code(code),
		_mnemonic(std::move(mnemonic))
	{
		ASSERT(cycles >= 4);
		ASSERT(cycles % 4 == 0);
		ASSERT(extra_cycles >= 0);
		ASSERT(extra_cycles % 4 == 0);
		ASSERT(0 <= extra_bytes && extra_bytes <= 2);
	}

	void execute(z80_cpu &cpu) const { _code(cpu); }
	const std::string &mnemonic() const { return _mnemonic; }
	int extra_bytes() const { return _extra_bytes; }
	int cycles() const { return _cycles; }
	int extra_cycles() const { return _extra_cycles; }

private:
	const int _extra_bytes;
	const int _cycles;
	const int _extra_cycles;  // cycles if the condition was true
	const code _code;
	const std::string _mnemonic;
};

using opcode_table = std::array<const opcode, 0x100>;
extern const opcode_table opcodes;
extern const opcode_table cb_opcodes;

}
