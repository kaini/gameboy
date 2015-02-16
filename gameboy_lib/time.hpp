#pragma once
#include <chrono>

namespace gb {

// cputime(1) == nanoseconds(1 / 2^23) == nanoseconds(119.209...)
using cputime = std::chrono::duration<long long, std::ratio<1, 8388608>>;

}
