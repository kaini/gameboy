#pragma once
#include "rom.hpp"
#include "video.hpp"
#include <thread>
#include <atomic>
#include <condition_variable>

namespace gb
{

class gb_thread
{
public:
	gb_thread();
	~gb_thread();

	/** Starts the thread. */
	void start(gb::rom rom);
	/** Stops the thread. */
	void stop();

	/** Returns a copy of the current video controller state. */
	video::raw_image fetch_image();

private:
	void run(gb::rom rom);

	bool _running;
	std::thread _thread;
	std::atomic<bool> _want_stop;

	std::mutex _image_mutex;
	std::condition_variable _got_image_cv;
	bool _got_image;
	video::raw_image _image;
};

}
