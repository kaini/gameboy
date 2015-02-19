#pragma once

namespace bit
{

template <typename T, typename M>
inline T &set(T &value, M mask)
{
	return value |= mask;
}

template <typename T, typename M>
inline T &clear(T &value, M mask)
{
	return value &= ~mask;
}

template <typename T, typename M>
inline bool test(T value, M mask)
{
	return (value & mask) == mask;
}

}
