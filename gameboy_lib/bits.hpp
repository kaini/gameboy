#pragma once

namespace bit
{

template <typename T>
T &set(T &value, T mask)
{
	return value |= mask;
}

template <typename T>
T &clear(T &value, T mask)
{
	return value &= ~mask;
}

template <typename T>
bool test(T value, T mask)
{
	return (value & mask) == mask;
}

}
