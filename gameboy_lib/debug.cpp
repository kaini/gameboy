#include "debug.hpp"
#include <Windows.h>

void debug_impl(std::ostringstream &ss)
{
	ss << std::endl;
	OutputDebugStringA(ss.str().c_str());
}

