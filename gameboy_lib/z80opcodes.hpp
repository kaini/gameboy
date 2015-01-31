#pragma once
#include <string>
#include <array>
#include <memory>

namespace gb
{
class z80_cpu;

// TODO remove inheritance with function field but name ugly :(
class opcode
{
public:
	opcode(std::string mnemonic, int extra_bytes, int cycles);
	virtual ~opcode() = default;

	virtual void execute(z80_cpu &cpu) const = 0;

	const std::string &mnemonic() const { return _mnemonic; }
	const int extra_bytes() const { return _extra_bytes; }
	const int cycles() const { return _cycles; }

private:
	const std::string _mnemonic;
	const int _extra_bytes;
	const int _cycles;
};

using opcode_table = std::array<std::unique_ptr<opcode>, 0x100>;
extern const opcode_table opcodes;
extern const opcode_table cb_opcodes;

}
