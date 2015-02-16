#pragma once
#include <cassert>

#ifdef ASSERT
	#error ASSERT already defined
#endif

#if _DEBUG

	#define ASSERT(expr) \
		do { \
			assert(expr); \
			__assume(expr); \
		} while (0)

	#define ASSERT_UNREACHABLE() \
		do { \
			assert(false && "unreachable"); \
			__assume(0); \
		} while (0)

#else

	#define ASSERT(expr) \
		do { \
			__assume(expr); \
		} while (0)

	#define ASSERT_UNREACHABLE() \
		do { \
			__assume(0); \
		} while (0)

#endif


