#include "gb_thread.hpp"
#include "z80.hpp"
#include "memory.hpp"
#include "rom.hpp"
#include "cart_rom_only.hpp"
#include "cart_mbc1.hpp"
#include "internal_ram.hpp"
#include "video.hpp"
#include "timer.hpp"
#include "joypad.hpp"
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
	gb::joypad joypad;

	// Make Memory
	gb::memory memory;
	memory.add_mapping(cartridge.get());
	memory.add_mapping(&internal_ram);
	memory.add_mapping(&video);
	memory.add_mapping(&joypad);
	memory.add_mapping(&timer);
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
