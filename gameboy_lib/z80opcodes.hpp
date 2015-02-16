#pragma once
#include <string>
#include <array>

namespace gb
{
class z80_cpu;

class opcode
{
	using code = void (*)(z80_cpu &cpu);

public:
	opcode(std::string mnemonic, int extra_bytes, int cycles, code code) :
		_mnemonic(std::move(mnemonic)),
		_extra_bytes(extra_bytes),
		_cycles(cycles),
		_code(code)
	{}

	void execute(z80_cpu &cpu) const { _code(cpu); }
	const std::string &mnemonic() const { return _mnemonic; }
	int extra_bytes() const { return _extra_bytes; }
	int cycles() const { return _cycles; }

private:
	const std::string _mnemonic;
	const int _extra_bytes;
	const int _cycles;
	const code _code;
};

using opcode_table = std::array<const opcode, 0x100>;
extern const opcode_table opcodes;
extern const opcode_table cb_opcodes;

}
