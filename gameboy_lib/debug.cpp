#include "debug.hpp"
#include <Windows.h>
#include <chrono>

void debug_impl(std::ostringstream &ss)
{
	using namespace std::chrono;
	auto time = duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();

	std::ostringstream msg;
	msg << "[" << time << "]  " << ss.str() << std::endl;
	OutputDebugStringA(msg.str().c_str());
}
