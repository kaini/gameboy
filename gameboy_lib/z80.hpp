#pragma once
#include "memory.hpp"
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
	template <> uint16_t read16<register16::af>() const { return _a << 8 | _f; }
	template <> uint16_t read16<register16::bc>() const { return _b << 8 | _c; }
	template <> uint16_t read16<register16::de>() const { return _d << 8 | _e; }
	template <> uint16_t read16<register16::hl>() const { return _h << 8 | _l; }
	template <> uint16_t read16<register16::sp>() const { return _sp; }
	template <> uint16_t read16<register16::pc>() const { return _pc; }

	template <register16 R> void write16(uint16_t value);
	template <> void write16<register16::af>(uint16_t value) { _a = value >> 8; _f = value & 0xF0; }
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
	template <> uint8_t read8<register8::f>() const { return _f; }
	template <> uint8_t read8<register8::h>() const { return _h; }
	template <> uint8_t read8<register8::l>() const { return _l; }

	template <register8 R> void write8(uint8_t value);
	template <> void write8<register8::a>(uint8_t value) { _a = value; }
	template <> void write8<register8::b>(uint8_t value) { _b = value; }
	template <> void write8<register8::c>(uint8_t value) { _c = value; }
	template <> void write8<register8::d>(uint8_t value) { _d = value; }
	template <> void write8<register8::e>(uint8_t value) { _e = value; }
	template <> void write8<register8::f>(uint8_t value) { _f = value & 0xF0; }
	template <> void write8<register8::h>(uint8_t value) { _h = value; }
	template <> void write8<register8::l>(uint8_t value) { _l = value; }

	template <cpu_flag F> bool get() const { return (_f & static_cast<uint8_t>(F)) != 0; }
	template <cpu_flag F> void set(bool value)
		{ if (value) { _f |= static_cast<uint8_t>(F); } else { _f &= ~static_cast<uint8_t>(F); } }

	void debug_print() const;

private:
	uint16_t _pc, _sp;
	uint8_t _f, _a, _c, _b, _e, _d, _l, _h;
};

class z80_cpu : private memory_mapping
{
public:
	static const uint16_t key1 = 0xFF4D;

	static const double clock_ns;
	static const double clock_fast_ns;

	z80_cpu(memory memory, register_file registers);
	double tick();

	register_file &registers() { return _registers; }
	const register_file &registers() const { return _registers; }
	gb::memory &memory() { return _memory; }
	const gb::memory &memory() const { return _memory; }
	uint8_t value8() const { return _value8; }
	uint16_t value16() const { return _value16; }

	/** Sets or resets the Interrupt Master Enable flag. */
	void set_ime(bool value) { _ime = value; }
	void post_interrupt(interrupt interrupt);
	void halt() { _halted = true; }

	/** Fast Mode. */
	bool double_speed() const { return _double_speed; }
	void stop();

private:
	register_file _registers;
	uint8_t _value8;
	uint16_t _value16;
	gb::memory _memory;
	bool _ime;
	bool _halted;

	bool _double_speed;
	bool _speed_switch;

	/** Memory mapping */
	// TODO pull interrupt registers here
	bool read8(uint16_t addr, uint8_t &value) const override;
	bool write8(uint16_t addr, uint8_t value) override;
};

}
