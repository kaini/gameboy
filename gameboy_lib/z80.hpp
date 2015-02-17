#pragma once
#include "memory.hpp"
#include "z80opcodes.hpp"
#include "time.hpp"
#include "bits.hpp"
#include <vector>
#include <cstdint>
#include <memory>

namespace gb
{

enum class register8
{
	a, f, b, c, d, e, h, l
};
std::string to_string(register8 r);

enum class register16
{
	af, bc, de, hl, sp, pc
};
std::string to_string(register16 r);

enum class cpu_flag : uint8_t
{
	/** CPU Flag: Zero. */
	z = 1 << 7,
	/** CPU Flag: Subtract. */
	n = 1 << 6,
	/** CPU Flag: Half Carry. */
	h = 1 << 5,
	/** CPU Flag: Carry. */
	c = 1 << 4,
};

enum class interrupt : uint8_t
{
	/** V-Blank */
	vblank = 1 << 0,
	/** LCDC */
	lcdc   = 1 << 1,
	/** Timer overflow */
	timer  = 1 << 2,
	/* Serial I/O complete */
	serial = 1 << 3,
	/** Pin 10-13 High to Low transition */
	pin    = 1 << 4,
};

class register_file
{
public:
	register_file();
	
	template <register16 R> uint16_t read16() const;
	template <> uint16_t read16<register16::af>() const { return _a << 8 | read8<register8::f>(); }
	template <> uint16_t read16<register16::bc>() const { return _b << 8 | _c; }
	template <> uint16_t read16<register16::de>() const { return _d << 8 | _e; }
	template <> uint16_t read16<register16::hl>() const { return _h << 8 | _l; }
	template <> uint16_t read16<register16::sp>() const { return _sp; }
	template <> uint16_t read16<register16::pc>() const { return _pc; }

	template <register16 R> void write16(uint16_t value);
	template <> void write16<register16::af>(uint16_t value) { _a = value >> 8; write8<register8::f>(static_cast<uint8_t>(value & 0xFF)); }
	template <> void write16<register16::bc>(uint16_t value) { _b = value >> 8; _c = value & 0xFF; }
	template <> void write16<register16::de>(uint16_t value) { _d = value >> 8; _e = value & 0xFF; }
	template <> void write16<register16::hl>(uint16_t value) { _h = value >> 8; _l = value & 0xFF; }
	template <> void write16<register16::sp>(uint16_t value) { _sp = value; }
	template <> void write16<register16::pc>(uint16_t value) { _pc = value; }

	template <register8 R> uint8_t read8() const;
	template <> uint8_t read8<register8::a>() const { return _a; }
	template <> uint8_t read8<register8::b>() const { return _b; }
	template <> uint8_t read8<register8::c>() const { return _c; }
	template <> uint8_t read8<register8::d>() const { return _d; }
	template <> uint8_t read8<register8::e>() const { return _e; }
	template <> uint8_t read8<register8::f>() const { return _flag_z << 7 | _flag_n << 6 | _flag_h << 5 | _flag_c << 4; }
	template <> uint8_t read8<register8::h>() const { return _h; }
	template <> uint8_t read8<register8::l>() const { return _l; }

	template <register8 R> void write8(uint8_t value);
	template <> void write8<register8::a>(uint8_t value) { _a = value; }
	template <> void write8<register8::b>(uint8_t value) { _b = value; }
	template <> void write8<register8::c>(uint8_t value) { _c = value; }
	template <> void write8<register8::d>(uint8_t value) { _d = value; }
	template <> void write8<register8::e>(uint8_t value) { _e = value; }
	template <> void write8<register8::f>(uint8_t value) { _flag_z = bit::test(value, 0x80); _flag_n = bit::test(value, 0x40); _flag_h = bit::test(value, 0x20); _flag_c = bit::test(value, 0x10);  }
	template <> void write8<register8::h>(uint8_t value) { _h = value; }
	template <> void write8<register8::l>(uint8_t value) { _l = value; }

	template <cpu_flag F> bool get() const;
	template <> bool get<cpu_flag::z>() const { return _flag_z; }
	template <> bool get<cpu_flag::n>() const { return _flag_n; }
	template <> bool get<cpu_flag::h>() const { return _flag_h; }
	template <> bool get<cpu_flag::c>() const { return _flag_c; }

	template <cpu_flag F> void set(bool value);
	template <> void set<cpu_flag::z>(bool value) { _flag_z = value; }
	template <> void set<cpu_flag::n>(bool value) { _flag_n = value; }
	template <> void set<cpu_flag::h>(bool value) { _flag_h = value; }
	template <> void set<cpu_flag::c>(bool value) { _flag_c = value; }

	void debug_print() const;

private:
	uint16_t _pc, _sp;
	uint8_t _a, _c, _b, _e, _d, _l, _h;
	bool _flag_z, _flag_n, _flag_h, _flag_c;
};

class z80_cpu : private memory_mapping
{
public:
	static const uint16_t key1 = 0xFF4D;

	static const cputime clock;
	static const cputime clock_fast;

	z80_cpu(memory memory, register_file registers);

	/** Simulation interface, always call them exactly in order! */
	cputime fetch_decode_execute();  // first
	cputime read();  // second
	cputime write();  // third

	/** Current CPU state. */
	register_file &registers() { return _registers; }
	const register_file &registers() const { return _registers; }
	gb::memory &memory() { return _memory; }
	const gb::memory &memory() const { return _memory; }

	/** Current opcode (valid after fetch_decode was called) */
	uint8_t value8() const { return _value8; }
	uint16_t value16() const { return _value16; }
	void set_jumped() { _jumped = true; }
	void set_temp(uint8_t v) { _temp = v; }
	uint8_t temp() const { return _temp; }

	/** Sets or resets the Interrupt Master Enable flag. */
	void set_ime(bool value) { _ime = value; }
	void post_interrupt(interrupt interrupt);
	void halt() { _halted = true; _opcode = nullptr; }

	/** Fast Mode. */
	bool double_speed() const { return _double_speed; }
	void stop();

	/** DMA. */
	void set_dma_mode(bool dma) { _memory.set_dma_mode(dma); }

private:
	register_file _registers;
	gb::memory _memory;
	bool _ime;
	bool _halted;

	uint8_t _value8;
	uint16_t _value16;
	const opcode *_opcode;
	bool _jumped;
	uint8_t _temp;

	bool _double_speed;
	bool _speed_switch;

	/** Memory mapping */
	// TODO pull interrupt registers here
	bool read8(uint16_t addr, uint8_t &value) const override;
	bool write8(uint16_t addr, uint8_t value) override;
};

}
