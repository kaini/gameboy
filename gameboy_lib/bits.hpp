#pragma once

namespace bit
{

template <typename T, typename M>
T &set(T &value, M mask)
{
	return value |= mask;
}

template <typename T, typename M>
T &clear(T &value, M mask)
{
	return value &= ~mask;
}

template <typename T, typename M>
bool test(T value, M mask)
{
	return (value & mask) == mask;
}

}
