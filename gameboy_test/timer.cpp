#include "timer.hpp"
#include "z80.hpp"
#include "z80opcodes.hpp"
#include "internal_ram.hpp"
#include <boost/test/unit_test.hpp>
#include <chrono>

using namespace std::chrono;

BOOST_AUTO_TEST_CASE(test_timer_div)
{
	gb::timer timer;
	gb::memory memory;
	memory.add_mapping(&timer);
	gb::z80_cpu cpu(std::move(memory), gb::register_file());

	timer.tick(cpu, nanoseconds(61034));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::div), 0);
	timer.tick(cpu, nanoseconds(1));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::div), 1);

	timer.tick(cpu, nanoseconds(61035 * 2));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::div), 3);

	memory.write8(gb::timer::div, 10);
	timer.tick(cpu, nanoseconds(60000));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::div), 0);
	timer.tick(cpu, nanoseconds(1034));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::div), 0);
	timer.tick(cpu, nanoseconds(1));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::div), 1);

	timer.tick(cpu, nanoseconds(61035 * 255));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::div), 0);

	// switch in double speed
	memory.write8(gb::z80_cpu::key1, 1);
	gb::opcodes[0x10 /* STOP */].execute(cpu);

	timer.tick(cpu, nanoseconds(30517));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::div), 0);
	timer.tick(cpu, nanoseconds(1));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::div), 1);
}

BOOST_AUTO_TEST_CASE(test_timer_tima_tma_tac)
{
	gb::timer timer;
	gb::internal_ram internal_ram;
	gb::memory memory;
	memory.add_mapping(&timer);
	memory.add_mapping(&internal_ram);
	gb::z80_cpu cpu(std::move(memory), gb::register_file());
	
	memory.write8(gb::timer::tima, 0);
	memory.write8(gb::timer::tma, 0);
	for (uint8_t i = 0; i < 4; ++i)
	{
		memory.write8(gb::timer::tac, i);
		timer.tick(cpu, seconds(1));
		BOOST_CHECK_EQUAL(memory.read8(gb::timer::tima), 0);
	}

	memory.write8(gb::timer::tac, 0x04);
	timer.tick(cpu, nanoseconds(244140));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::tima), 0);
	timer.tick(cpu, nanoseconds(1));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::tima), 1);

	memory.write8(gb::timer::tac, 0x05);
	timer.tick(cpu, nanoseconds(3814));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::tima), 1);
	timer.tick(cpu, nanoseconds(1));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::tima), 2);

	memory.write8(gb::timer::tac, 0x06);
	timer.tick(cpu, nanoseconds(15258));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::tima), 2);
	timer.tick(cpu, nanoseconds(1));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::tima), 3);

	memory.write8(gb::timer::tac, 0x07);
	timer.tick(cpu, nanoseconds(61034));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::tima), 3);
	timer.tick(cpu, nanoseconds(1));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::tima), 4);

	timer.tick(cpu, nanoseconds(60000));
	memory.write8(gb::timer::tima, 0xff);
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::tima), 0xff);
	BOOST_CHECK_EQUAL(memory.read8(gb::internal_ram::if_) & 0x4, 0);
	timer.tick(cpu, nanoseconds(1035));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::tima), 0);
	BOOST_CHECK_EQUAL(memory.read8(gb::internal_ram::if_) & 0x4, 0x4);

	memory.write8(gb::timer::tma, 0x44);
	memory.write8(gb::timer::tima, 0xff);
	timer.tick(cpu, nanoseconds(61035));
	BOOST_CHECK_EQUAL(memory.read8(gb::timer::tima), 0x44);
}
