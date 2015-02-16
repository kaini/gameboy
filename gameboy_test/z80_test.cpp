#include "z80.hpp"
#include <array>
#include <vector>
#include <boost/test/unit_test.hpp>
#include <unordered_set>
#include <cstdio>
#include <algorithm>

namespace
{

class test_memory : public gb::memory_mapping
{
public:
	test_memory(std::vector<uint8_t> code, std::unordered_set<uint16_t> writes={}) :
		_code(std::move(code)), _writes(writes)
	{
		std::fill(_ram.begin(), _ram.end(), 0);
	}

	bool read8(uint16_t addr, uint8_t &value) const override
	{
		if (addr < _code.size())
		{
			value = _code[addr];
			return true;
		}
		else
		{
			value = _ram[addr];
			return true;
		}
	}

	bool write8(uint16_t addr, uint8_t value) override
	{
		if (_writes.find(addr) == _writes.end())
		{
			BOOST_ERROR("invalid memory write");
		}
		_ram[addr] = value;
		return true;
	}

private:
	std::vector<uint8_t> _code;
	std::array<uint8_t, 0x10000> _ram;
	std::unordered_set<uint16_t> _writes;
};

}

static gb::z80_cpu run_cpu(test_memory *mapping, int instructions, gb::register_file registers=gb::register_file())
{
	gb::memory memory;
	memory.add_mapping(mapping);
	gb::z80_cpu cpu(std::move(memory), std::move(registers));
	cpu.registers() = registers;

	while (instructions--)
	{
		cpu.fetch_decode_execute();
		cpu.read();
		cpu.write();
	}

	return cpu;
}

template <gb::register8 R, uint8_t Opcode>
void test_intermediate_loads8_impl()
{
	for (int i = 0; i <= 255; ++i)
	{
		test_memory mem({Opcode, static_cast<uint8_t>(i)});
		const gb::register_file registers;
		const auto cpu = run_cpu(&mem, 1, registers);

		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::b>(), R == gb::register8::b ? i : 0);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::c>(), R == gb::register8::c ? i : 0);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::d>(), R == gb::register8::d ? i : 0);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::e>(), R == gb::register8::e ? i : 0);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::h>(), R == gb::register8::h ? i : 0);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::l>(), R == gb::register8::l ? i : 0);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::a>(), R == gb::register8::a ? i : 0);
		BOOST_CHECK_EQUAL(cpu.registers().read16<gb::register16::sp>(), 0);
		BOOST_CHECK_EQUAL(cpu.registers().read16<gb::register16::pc>(), 2);
	}
}

BOOST_AUTO_TEST_CASE(test_intermediate_loads8)
{
	test_intermediate_loads8_impl<gb::register8::b, 0x06>();
	test_intermediate_loads8_impl<gb::register8::c, 0x0e>();
	test_intermediate_loads8_impl<gb::register8::d, 0x16>();
	test_intermediate_loads8_impl<gb::register8::e, 0x1e>();
	test_intermediate_loads8_impl<gb::register8::h, 0x26>();
	test_intermediate_loads8_impl<gb::register8::l, 0x2e>();
	test_intermediate_loads8_impl<gb::register8::a, 0x3e>();

	for (int i = 0; i <= 255; ++i)
	{
		test_memory mem({0x36, static_cast<uint8_t>(i)}, {0x1234});  // ld (hl),'T'
		gb::register_file registers;
		registers.write16<gb::register16::hl>(0x1234);
		const auto cpu = run_cpu(&mem, 1, registers);

		uint8_t value = 0;
		mem.read8(0x1234, value);
		BOOST_CHECK_EQUAL(value, i);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::b>(), 0);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::c>(), 0);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::d>(), 0);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::e>(), 0);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::h>(), 0x12);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::l>(), 0x34);
		BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::a>(), 0);
		BOOST_CHECK_EQUAL(cpu.registers().read16<gb::register16::sp>(), 0);
		BOOST_CHECK_EQUAL(cpu.registers().read16<gb::register16::pc>(), 2);
	}
}

static void test_add8_impl(uint8_t a, uint8_t b, bool carry, bool half_carry)
{
	gb::register_file registers;
	registers.write8<gb::register8::a>(a);
	registers.write8<gb::register8::b>(b);
	registers.set<gb::cpu_flag::c>(true);
	test_memory mem({0x80});
	const auto cpu = run_cpu(&mem, 1, registers);

	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::c>(), carry);
	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::h>(), half_carry);
	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::n>(), false);
	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::z>(), static_cast<uint8_t>(a + b) == 0);
	BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::a>(), static_cast<uint8_t>(a + b));
	BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::b>(), b);
}

BOOST_AUTO_TEST_CASE(test_add8)
{
	test_add8_impl(0x55, 0xAA, false, false);
	test_add8_impl(0xA0, 0x20, false, false);
	test_add8_impl(0x0A, 0x02, false, false);

	test_add8_impl(0x80, 0x80, true, false);
	test_add8_impl(0xC0, 0x40, true, false);

	test_add8_impl(0x08, 0x08, false, true);
	test_add8_impl(0x0C, 0x04, false, true);

	test_add8_impl(0xFF, 0x01, true, true);
	test_add8_impl(0x88, 0x88, true, true);
	test_add8_impl(0xF8, 0x08, true, true);
}

static void test_adc8_impl(bool init_carry, uint8_t a, uint8_t b, bool carry, bool half_carry)
{
	gb::register_file registers;
	registers.write8<gb::register8::a>(a);
	registers.write8<gb::register8::b>(b);
	registers.set<gb::cpu_flag::c>(init_carry);
	test_memory mem({0x88});
	const auto cpu = run_cpu(&mem, 1, registers);

	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::c>(), carry);
	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::h>(), half_carry);
	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::n>(), false);
	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::z>(), static_cast<uint8_t>(a + b + (init_carry ? 1 : 0)) == 0);
	BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::a>(), static_cast<uint8_t>(a + b + (init_carry ? 1 : 0)));
	BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::b>(), b);
}

BOOST_AUTO_TEST_CASE(test_adc8)
{
	test_adc8_impl(false, 0x55, 0xAA, false, false);
	test_adc8_impl(false, 0xA0, 0x20, false, false);
	test_adc8_impl(false, 0x0A, 0x02, false, false);
	test_adc8_impl(false, 0x80, 0x80, true, false);
	test_adc8_impl(false, 0xC0, 0x40, true, false);
	test_adc8_impl(false, 0x08, 0x08, false, true);
	test_adc8_impl(false, 0x0C, 0x04, false, true);
	test_adc8_impl(false, 0xFF, 0x01, true, true);
	test_adc8_impl(false, 0x88, 0x88, true, true);
	test_adc8_impl(false, 0xF8, 0x08, true, true);
	test_adc8_impl(false, 0xF4, 0xFB, true, false);

	test_adc8_impl(true, 0xFF, 0x00, true, true);
	test_adc8_impl(true, 0x0F, 0x00, false, true);
	test_adc8_impl(true, 0xFF, 0xFF, true, true);
	test_adc8_impl(true, 0x00, 0x00, false, false);
	test_adc8_impl(true, 0xEF, 0x10, true, true);
	test_adc8_impl(true, 0x0F, 0x0F, false, true);
	test_adc8_impl(true, 0x00, 0x0F, false, true);
	test_adc8_impl(true, 0x04, 0x0B, false, true);
}

static void test_sub8_impl(uint8_t a, uint8_t b, bool carry, bool half_carry)
{
	gb::register_file registers;
	registers.write8<gb::register8::a>(a);
	registers.write8<gb::register8::b>(b);
	registers.set<gb::cpu_flag::c>(true);
	test_memory mem({0x90});
	const auto cpu = run_cpu(&mem, 1, registers);

	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::c>(), carry);
	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::h>(), half_carry);
	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::n>(), true);
	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::z>(), static_cast<uint8_t>(a - b) == 0);
	BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::a>(), static_cast<uint8_t>(a - b));
	BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::b>(), b);
}

BOOST_AUTO_TEST_CASE(test_sub8)
{
	test_sub8_impl(0xFF, 0xFF, false, false);
	test_sub8_impl(0x80, 0x01, false, true);
	test_sub8_impl(0x88, 0x11, false, false);
	test_sub8_impl(0x00, 0x01, true, true);
	test_sub8_impl(0x00, 0xFF, true, true);
	test_sub8_impl(0x8F, 0x9F, true, false);
}

static void test_sbc8_impl(bool init_carry, uint8_t a, uint8_t b, bool carry, bool half_carry)
{
	gb::register_file registers;
	registers.write8<gb::register8::a>(a);
	registers.write8<gb::register8::b>(b);
	registers.set<gb::cpu_flag::c>(init_carry);
	test_memory mem({0x98});
	const auto cpu = run_cpu(&mem, 1, registers);

	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::c>(), carry);
	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::h>(), half_carry);
	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::n>(), true);
	BOOST_CHECK_EQUAL(cpu.registers().get<gb::cpu_flag::z>(), static_cast<uint8_t>(a - b - (init_carry ? 1 : 0)) == 0);
	BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::a>(), static_cast<uint8_t>(a - b - (init_carry ? 1 : 0)));
	BOOST_CHECK_EQUAL(cpu.registers().read8<gb::register8::b>(), b);
}

BOOST_AUTO_TEST_CASE(test_sbc8)
{
	test_sbc8_impl(false, 0xFF, 0xFF, false, false);
	test_sbc8_impl(false, 0x80, 0x01, false, true);
	test_sbc8_impl(false, 0x88, 0x11, false, false);
	test_sbc8_impl(false, 0x00, 0x01, true, true);
	test_sbc8_impl(false, 0x00, 0xFF, true, true);
	test_sbc8_impl(false, 0x8F, 0x9F, true, false);

	test_sbc8_impl(true, 0xFF, 0xFF, true, true);
	test_sbc8_impl(true, 0x1F, 0x0F, false, true);
	test_sbc8_impl(true, 0x00, 0x00, true, true);
	test_sbc8_impl(true, 0x00, 0xFF, true, true);
}

BOOST_AUTO_TEST_CASE(test_daa)
{
	test_memory mem({0x27});

	for (unsigned int a = 0; a <= 0xFF; ++a)
	{
		for (unsigned int f = 0; f <= 0xF0; f += 0x10)
		{
			gb::register_file registers;
			registers.write8<gb::register8::a>(static_cast<uint8_t>(a));
			registers.write8<gb::register8::f>(static_cast<uint8_t>(f));
			bool before_c = registers.get<gb::cpu_flag::c>();
			bool before_h = registers.get<gb::cpu_flag::h>();
			bool before_n = registers.get<gb::cpu_flag::n>();
			bool before_z = registers.get<gb::cpu_flag::z>();
			const auto cpu = run_cpu(&mem, 1, std::move(registers));

			
			uint8_t result = cpu.registers().read8<gb::register8::a>();
			bool after_c = cpu.registers().get<gb::cpu_flag::c>();
			bool after_h = cpu.registers().get<gb::cpu_flag::h>();
			bool after_n = cpu.registers().get<gb::cpu_flag::n>();
			bool after_z = cpu.registers().get<gb::cpu_flag::z>();
			uint8_t right_result = static_cast<uint8_t>(a);

			// http://www.z80.info/z80syntx.htm#DAA
			if (before_n)
			{
				if (before_c)
				{
					if (before_h)
					{
						if ((a & 0xF0) >= 0x60 && (a & 0x0F) >= 0x06)
						{
							right_result += 0x9A;
							BOOST_CHECK_EQUAL(after_c, true);
						}
						else
						{
							continue;
						}
					}
					else
					{
						if ((a & 0xF0) >= 0x70 && (a & 0x0F) <= 0x09)
						{
							right_result += 0xA0;
							BOOST_CHECK_EQUAL(after_c, true);
						}
						else
						{
							continue;
						}
					}
				}
				else  // !before_c
				{
					if (before_h)
					{
						if ((a & 0xF0) <= 0x80 && (a & 0x0F) >= 0x06)
						{
							right_result += 0xFA;
							BOOST_CHECK_EQUAL(after_c, false);
						}
						else
						{
							continue;
						}
					}
					else
					{
						if ((a & 0xF0) <= 0x90 && (a & 0x0F) <= 0x09)
						{
							BOOST_CHECK_EQUAL(after_c, false);
						}
						else
						{
							continue;
						}
					}
				}
			}
			else  // !before_n
			{
				if (before_c)
				{
					if (before_h)
					{
						if ((a & 0xF0) <= 0x30 && (a & 0x0F) <= 0x03)
						{
							right_result += 0x66;
							BOOST_CHECK_EQUAL(after_c, true);
						}
						else
						{
							continue;
						}
					}
					else
					{
						if ((a & 0xF0) <= 0x20 && (a & 0x0F) <= 0x09)
						{
							right_result += 0x60;
							BOOST_CHECK_EQUAL(after_c, true);
						}
						else if ((a & 0xF0) <= 0x20 && (a & 0x0F) >= 0x0A)
						{
							right_result += 0x66;
							BOOST_CHECK_EQUAL(after_c, true);
						}
						else
						{
							continue;
						}
					}
				}
				else  // !before_c
				{
					if (before_h)
					{
						if ((a & 0xF0) <= 0x90 && (a & 0x0F) <= 0x03)
						{
							right_result += 0x06;
							BOOST_CHECK_EQUAL(after_c, false);
						}
						else if ((a & 0xF0) >= 0xA0 && (a & 0x0F) <= 0x03)
						{
							right_result += 0x66;
							BOOST_CHECK_EQUAL(after_c, true);
						}
						else
						{
							continue;
						}
					}
					else // !before_h
					{
						if ((a & 0xF0) <= 0x90 && (a & 0x0F) <= 0x09)
						{
							BOOST_CHECK_EQUAL(after_c, false);
						}
						else if ((a & 0xF0) <= 0x80 && (a & 0x0F) >= 0x0A)
						{
							right_result += 0x06;
							BOOST_CHECK_EQUAL(after_c, false);
						}
						else if ((a & 0xF0) >= 0xA0 && (a & 0x0F) <= 0x09)
						{
							right_result += 0x60;
							BOOST_CHECK_EQUAL(after_c, true);
						}
						else if ((a & 0xF0) >= 0x90 && (a & 0x0F) >= 0x0A)
						{
							right_result += 0x66;
							BOOST_CHECK_EQUAL(after_c, true);
						}
						else
						{
							BOOST_ERROR("unreachable");
						}
					}
				}
			}

			BOOST_CHECK_EQUAL(result, right_result);
			BOOST_CHECK_EQUAL(after_z, right_result == 0);
			BOOST_CHECK_EQUAL(after_n, before_n);
			BOOST_CHECK_EQUAL(after_h, false);
		}
	}
}
