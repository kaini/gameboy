#pragma once
#include "assert.hpp"
#include <string>
#include <array>

namespace gb
{
class z80_cpu;

struct opcode
{
	using code = void (*)(z80_cpu &cpu);

	opcode(std::string mnemonic, int extra_bytes, int cycles, code base_code, int jump_cycles=0,
			code read_code=nullptr, code write_code=nullptr) :
		extra_bytes(extra_bytes),
		cycles(cycles),
		jump_cycles(jump_cycles),
		base_code(base_code == nullptr ? &noop : base_code),
		read_code(read_code),
		write_code(write_code),
		mnemonic(std::move(mnemonic))
	{
		ASSERT(cycles >= 4);
		ASSERT(cycles % 4 == 0);
		ASSERT(jump_cycles >= 0);
		ASSERT(jump_cycles % 4 == 0);
		ASSERT(0 <= extra_bytes && extra_bytes <= 2);
	}

	const int extra_bytes;
	const int cycles;
	const int jump_cycles;
	const code base_code;  // base code, always executed, taxed `cycles` cycles
	                       // if the PC changed `jump_cycles` cycles are taxed as well
	const code read_code;  // read code, taxed 1 cycle if != nullptr
	const code write_code;  // write code, taxed 1 cycle if != nullptr
	const std::string mnemonic;

private:
	static void noop(z80_cpu &) {}
};

using opcode_table = std::array<const opcode, 0x100>;
extern const opcode_table opcodes;
extern const opcode_table cb_opcodes;

}
