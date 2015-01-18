#include "gb_thread.hpp"
#include "z80.hpp"
#include "memory.hpp"
#include "rom.hpp"
#include "cart_rom_only.hpp"
#include "cart_mbc1.hpp"
#include "internal_ram.hpp"
#include "video.hpp"
#include "timer.hpp"
#include <cstdlib>
#include <vector>
#include <fstream>
#include <cassert>
#include <memory>

gb::gb_thread::gb_thread() :
	_running(false), _got_image(true)
{
}

gb::gb_thread::~gb_thread()
{
	stop();
}

void gb::gb_thread::start(gb::rom rom)
{
	assert(!_running);
	_running = true;
	_want_stop = false;
	_thread = std::thread(&gb_thread::run, this, std::move(rom));
}

void gb::gb_thread::stop()
{
	if (_running)
	{
		_want_stop = true;
		_running = false;
		_thread.join();
	}
}

gb::video::raw_image gb::gb_thread::fetch_image()
{
	std::unique_lock<std::mutex> lock(_image_mutex);
	_got_image = false;
	_got_image_cv.wait(lock, [this] { return _got_image; });
	return std::move(_image);
}

void gb::gb_thread::run(gb::rom rom)
{
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
	default:
		// TODO print error?
		return;
	}

	// Misc. Hardware
	gb::internal_ram internal_ram;
	gb::video video;
	gb::timer timer;

	// Make Memory
	gb::memory memory;
	memory.add_mapping(cartridge.get());
	memory.add_mapping(&internal_ram);
	memory.add_mapping(&video);
	memory.add_mapping(&timer);

	// Make Cpu
	gb::z80_cpu cpu(std::move(memory));

	// Let's go :)
	while (!_want_stop)
	{
		int ns = cpu.tick();
		video.tick(cpu, ns);
		timer.tick(cpu, ns);

		{
			std::lock_guard<std::mutex> lock(_image_mutex);
			if (!_got_image)
			{
				if (video.is_enabled())
					_image = video.image();
				else
					for (auto &row : _image)
						for (auto &pixel : row)
							std::fill(pixel.begin(), pixel.end(), 0xFF);

				_got_image = true;
				_got_image_cv.notify_one();
			}
		}
	}
}
