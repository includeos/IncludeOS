
#pragma once
#include "clocks.hpp"
#include <cstdint>

namespace x86
{
  struct CMOS_clock
  {
    static void init();
    static uint64_t system_time();
    static timespec wall_clock();
    static util::KHz      get_tsc_khz();
  };
}
