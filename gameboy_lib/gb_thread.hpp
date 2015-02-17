#pragma once
#include "rom.hpp"
#include "video.hpp"
#include "internal_ram.hpp"
#include "timer.hpp"
#include "joypad.hpp"
#include "sound.hpp"
#include "z80.hpp"
#include <thread>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <vector>
#include <future>

namespace gb
{

struct unsupported_rom_exception : public std::runtime_error
{
	unsupported_rom_exception(const std::string &what) : std::runtime_error(what) {}
};

struct gb_hardware
{
	gb_hardware(rom rom);

	// This Type is very unmovabe/copyable because pointers everywhere!
	gb_hardware(gb_hardware &&) = delete;
	gb_hardware(const gb_hardware &) = delete;
	gb_hardware &operator=(gb_hardware &&) = delete;
	gb_hardware &operator=(const gb_hardware &) = delete;

	cputime tick();

	std::unique_ptr<gb::memory_mapping> cartridge;
	gb::internal_ram internal_ram;
	gb::video video;
	gb::timer timer;
	gb::joypad joypad;
	gb::sound sound;
	std::unique_ptr<gb::z80_cpu> cpu;
};

class gb_thread
{
public:
	gb_thread();
	~gb_thread();

	/** Starts the thread. */
	void start(gb::rom rom);
	/** Joins the thread. */
	void join();

	/** Posts a request to stop the thread. */
	void post_stop();
	/** Posts a request to get the current image. */
	std::future<video::raw_image> post_get_image();

private:
	// Client Data
	bool _running;
	std::thread _thread;

	// Server Data
	void run();
	std::unique_ptr<gb_hardware> _gb;

	// Shared Data
	using command = std::function<void ()>;
	std::mutex _mutex;
	std::vector<command> _command_queue;
};

}
