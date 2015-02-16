#include "timer.hpp"
#include "z80.hpp"
#include "z80opcodes.hpp"
#include "internal_ram.hpp"
#include <boost/test/unit_test.hpp>
#include <chrono>

using namespace std::chrono;
using gb::cputime;

BOOST_AUTO_TEST_CASE(test_timer_div)
{
	gb::timer timer;
	gb::memory memory;
	memory.add_mapping(&timer);
	gb::z80_cpu cpu(std::move(memory), gb::register_file());

	timer.tick(cpu, cputime(511));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::div), 0);
	timer.tick(cpu, cputime(1));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::div), 1);

	timer.tick(cpu, cputime(512 * 2));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::div), 3);

	memory.write8(gb::timer::div, 10);
	timer.tick(cpu, cputime(500));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::div), 0);
	timer.tick(cpu, cputime(11));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::div), 0);
	timer.tick(cpu, cputime(1));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::div), 1);

	timer.tick(cpu, cputime(512 * 255));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::div), 0);

	// switch in double speed
	cpu.memory().write8(gb::z80_cpu::key1, 1);
	cpu.stop();

	timer.tick(cpu, cputime(255));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::div), 0);
	timer.tick(cpu, cputime(1));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::div), 1);
}

BOOST_AUTO_TEST_CASE(test_timer_tima_tma_tac)
{
	gb::timer timer;
	gb::internal_ram internal_ram;
	gb::memory memory;
	memory.add_mapping(&timer);
	memory.add_mapping(&internal_ram);
	gb::z80_cpu cpu(std::move(memory), gb::register_file());
	
	cpu.memory().write8(gb::timer::tima, 0);
	cpu.memory().write8(gb::timer::tma, 0);
	for (uint8_t i = 0; i < 4; ++i)
	{
		cpu.memory().write8(gb::timer::tac, i);
		timer.tick(cpu, seconds(1));
		BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::tima), 0);
	}

	cpu.memory().write8(gb::timer::tac, 0x04);
	timer.tick(cpu, cputime(2047));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::tima), 0);
	timer.tick(cpu, cputime(1));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::tima), 1);

	cpu.memory().write8(gb::timer::tac, 0x05);
	timer.tick(cpu, cputime(31));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::tima), 1);
	timer.tick(cpu, cputime(1));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::tima), 2);

	cpu.memory().write8(gb::timer::tac, 0x06);
	timer.tick(cpu, cputime(127));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::tima), 2);
	timer.tick(cpu, cputime(1));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::tima), 3);

	cpu.memory().write8(gb::timer::tac, 0x07);
	timer.tick(cpu, cputime(511));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::tima), 3);
	timer.tick(cpu, cputime(1));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::tima), 4);

	timer.tick(cpu, cputime(500));
	cpu.memory().write8(gb::timer::tima, 0xff);
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::tima), 0xff);
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::internal_ram::if_) & 0x4, 0);
	timer.tick(cpu, cputime(12));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::tima), 0);
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::internal_ram::if_) & 0x4, 0x4);

	cpu.memory().write8(gb::timer::tma, 0x44);
	cpu.memory().write8(gb::timer::tima, 0xff);
	timer.tick(cpu, cputime(512));
	BOOST_CHECK_EQUAL(cpu.memory().read8(gb::timer::tima), 0x44);
}
