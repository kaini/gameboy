#include "gb_thread.hpp"
#include "z80.hpp"
#include "memory.hpp"
#include "rom.hpp"
#include "cart_rom_only.hpp"
#include "cart_mbc1.hpp"
#include "cart_mbc5.hpp"
#include "internal_ram.hpp"
#include "video.hpp"
#include "timer.hpp"
#include "joypad.hpp"
#include "sound.hpp"
#include "debug.hpp"
#include "assert.hpp"
#include <cstdlib>
#include <vector>
#include <fstream>
#include <memory>
#include <chrono>

namespace
{
class stop_exception {};
}

gb::gb_thread::gb_thread() :
	_running(false)
{
}

gb::gb_thread::~gb_thread()
{
	post_stop();
	join();
}

void gb::gb_thread::start(gb::rom rom)
{
	ASSERT(!_running);
	_running = true;
	_thread = std::thread(&gb_thread::run, this, std::move(rom));
}

void gb::gb_thread::join()
{
	if (_running)
	{
		_thread.join();
	}
}

void gb::gb_thread::post_stop()
{
	command fn([] (video &){ 
		throw stop_exception();
	});

	std::lock_guard<std::mutex> lock(_mutex);
	_command_queue.emplace_back(std::move(fn));
}

std::future<gb::video::raw_image> gb::gb_thread::post_get_image()
{
	// TODO use capture by move (Visual Studio 2015/C++14)
	auto promise = std::make_shared<std::promise<video::raw_image>>();
	auto future = promise->get_future();
	command fn([promise] (video &video) {
		promise->set_value(video.image());
	});

	std::lock_guard<std::mutex> lock(_mutex);
	_command_queue.emplace_back(std::move(fn));
	return future;
}

void gb::gb_thread::run(gb::rom rom)
{
	using namespace std::chrono;

	using clock = steady_clock;
	static_assert(clock::is_monotonic, "clock not monotonic");
	static_assert(clock::is_steady, "clock not steady");
	static_assert(std::ratio_less_equal<clock::period, std::ratio_multiply<std::ratio<100>, std::nano>>::value,
		"clock too inaccurate (period > 100ns)");

	if (ASSERT_ENABLED)
		debug("WARNING: asserts are enabled!");

	// Make Cartridge
	std::unique_ptr<gb::memory_mapping> cartridge;
	switch (rom.cartridge())
	{
	case 0x00:
		cartridge = std::make_unique<gb::cart_rom_only>(std::move(rom));
		break;
	case 0x01:
		cartridge = std::make_unique<gb::cart_mbc1>(std::move(rom));
		break;
	case 0x19:
		cartridge = std::make_unique<gb::cart_mbc5>(std::move(rom));
		break;
	default:
		return;
	}

	// Misc. Hardware
	gb::internal_ram internal_ram;
	gb::video video;
	gb::timer timer;
	gb::joypad joypad;
	gb::sound sound;

	// Make Memory
	gb::memory memory;
	memory.add_mapping(cartridge.get());
	memory.add_mapping(&internal_ram);
	memory.add_mapping(&video);
	memory.add_mapping(&joypad);
	memory.add_mapping(&timer);
	memory.add_mapping(&sound);
	memory.write8(0xff05, 0x00);
	memory.write8(0xff06, 0x00);
	memory.write8(0xff07, 0x00);
	memory.write8(0xff10, 0x80);
	memory.write8(0xff11, 0xbf);
	memory.write8(0xff12, 0xf3);
	memory.write8(0xff14, 0xbf);
	memory.write8(0xff16, 0x3f);
	memory.write8(0xff17, 0x00);
	memory.write8(0xff19, 0xbf);
	memory.write8(0xff1a, 0x7f);
	memory.write8(0xff1b, 0xff);
	memory.write8(0xff1c, 0x9f);
	memory.write8(0xff1e, 0xbf);
	memory.write8(0xff20, 0xff);
	memory.write8(0xff21, 0x00);
	memory.write8(0xff22, 0x00);
	memory.write8(0xff23, 0xbf);
	memory.write8(0xff24, 0x77);
	memory.write8(0xff25, 0xf3);
	memory.write8(0xff26, 0xf1);
	memory.write8(0xff40, 0x91);
	memory.write8(0xff42, 0x00);
	memory.write8(0xff43, 0x00);
	memory.write8(0xff45, 0x00);
	memory.write8(0xff47, 0xfc);
	memory.write8(0xff48, 0xff);
	memory.write8(0xff49, 0xff);
	memory.write8(0xff4a, 0x00);
	memory.write8(0xff4b, 0x00);
	memory.write8(0xffff, 0x00);

	// Register file
	gb::register_file registers;
	registers.write8<register8::a>(0x11);
	registers.write8<register8::f>(0xb0);
	registers.write16<register16::bc>(0x0013);
	registers.write16<register16::de>(0x00d8);
	registers.write16<register16::hl>(0x014d);
	registers.write16<register16::sp>(0xfffe);
	registers.write16<register16::pc>(0x0100);

	// Make Cpu
	gb::z80_cpu cpu(std::move(memory), std::move(registers));

	// Let's go :)
	std::vector<command> current_commands;

	cputime gb_time(0);
	auto real_time_start = clock::now();

	cputime performance_gb_time(0);
	nanoseconds performance_sleep_time(0);
	auto performance_start = clock::now();

	try
	{
		while (true)
		{
			// Command stream
			{
				std::lock_guard<std::mutex> lock(_mutex);
				if (!_command_queue.empty())
				{
					std::swap(current_commands, _command_queue);
				}
			}

			if (!current_commands.empty())
			{
				for (const auto &command : current_commands)
				{
					command(video);
				}
				current_commands.clear();
			}

			// Simulation itself
			const auto time_fde = cpu.fetch_decode_execute();
			timer.tick(cpu, time_fde);

			const auto time_r = cpu.read();
			timer.tick(cpu, time_r);

			const auto time_w = cpu.write();
			timer.tick(cpu, time_w);

			const auto time = time_fde + time_r + time_w;
			video.tick(cpu, time);

			// Time bookkeeping
			gb_time += time;
			const auto real_time = clock::now() - real_time_start;
			const auto drift = duration_cast<nanoseconds>(gb_time) - real_time;
			if (drift > milliseconds(5))
			{
				// Simulation is too fast
				const auto sleep_start = clock::now();
				std::this_thread::sleep_for(drift);
				performance_sleep_time += (clock::now() - sleep_start);

				const auto new_current_time = clock::now();
				const auto new_real_time = new_current_time - real_time_start;
				const auto new_drift = gb_time - duration_cast<cputime>(new_real_time);
				gb_time = new_drift;
				real_time_start = new_current_time;
			}
			else if (drift < milliseconds(-100))
			{
				// Simulation is too slow (reset counter to avoid an endless accumulation of negaitve time)
				// This is a resync-attempt in case of a spike and avoids underflow
				gb_time = cputime(0);
				real_time_start = clock::now();
			}

			// Performance-o-meter
			performance_gb_time += time;
			const auto performance_now = clock::now();
			const auto performance_real_time = performance_now - performance_start;
			if (performance_real_time > seconds(10))
			{
				const auto accuracy =
					duration_cast<milliseconds>(performance_gb_time - performance_real_time).count();
				const double speed =
					static_cast<double>(duration_cast<nanoseconds>(performance_gb_time).count()) /
					static_cast<double>(duration_cast<nanoseconds>(performance_real_time - performance_sleep_time).count()) *
					100.0;
				debug("PERF: simulation drift in the last 10 s was ~", accuracy, " ms");
				debug("PERF: simulation speed in the last 10 s was ", speed, " % of required speed");
				if (speed < 110.0)
				{
					debug("PERF WARNING: simulation speed is too low (< 110 %)");
				}
				performance_sleep_time = seconds(0);
				performance_gb_time = cputime(0);
				performance_start = performance_now;
			}
		}
	}
	catch (const stop_exception &)
	{
		// this might be ugly but it works well :)
	}
}
