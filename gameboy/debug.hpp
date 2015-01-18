#pragma once
#include <sstream>

void debug_impl(std::ostringstream &ss);

template <typename T, typename ...Rest>
void debug_impl(std::ostringstream &ss, T &&t, Rest &&...rest)
{
	ss << t;
	debug_impl(ss, rest...);
}

template <typename ...Args>
void debug(Args &&...ts)
{
	std::ostringstream message;
	debug_impl(message, ts...);
}
