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
#include <cstdlib>
#include <vector>
#include <fstream>
#include <cassert>
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
	assert(!_running);
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
	nanoseconds real_time(0);
	nanoseconds gb_time(0);
	auto last_real_time = clock::now();

	try
	{
		while (true)
		{
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

			auto ns = cpu.tick();
			video.tick(cpu, ns);
			timer.tick(cpu, ns);

			// Time bookkeeping
			gb_time += ns;
			auto current_time = clock::now();
			real_time += (current_time - last_real_time);
			last_real_time = current_time;

			auto min_time = std::min(gb_time, real_time);
			gb_time -= min_time;
			real_time -= min_time;
			
			if (gb_time > nanoseconds(0))
			{
				// simulation is too fast
				if (gb_time > milliseconds(15))
				{
					std::this_thread::sleep_for(gb_time);
				}
			}
			else if (real_time > milliseconds(100))
			{
				// simulation is too slow (reset counter)
				debug("simulation is too slow!");
				real_time = nanoseconds(0);
			}
		}
	}
	catch (const stop_exception &)
	{
		// this might be ugly but it works well :)
	}
}
