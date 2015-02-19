#include "debug.hpp"
#include <chrono>

#ifdef _MSC_VER
	#include <Windows.h>
#else
	#include <iostream>
#endif

void debug_impl(std::ostringstream &ss)
{
	using namespace std::chrono;
	auto time = duration_cast<seconds>(steady_clock::now().time_since_epoch()).count();

	std::ostringstream msg;
	msg << "[" << time << "]  " << ss.str() << std::endl;

#ifdef _MSC_VER
	OutputDebugStringA(msg.str().c_str());
#else
	std::cerr << msg.str();
#endif
}
