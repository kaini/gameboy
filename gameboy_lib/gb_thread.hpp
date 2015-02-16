#pragma once
#include "rom.hpp"
#include "video.hpp"
#include <thread>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <vector>
#include <future>

namespace gb
{

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
	void run(gb::rom rom);

	bool _running;
	std::thread _thread;

	using command = std::function<void (video &)>;
	std::mutex _mutex;
	std::vector<command> _command_queue;
};

}
