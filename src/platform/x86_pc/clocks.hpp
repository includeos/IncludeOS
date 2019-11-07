
#pragma once
#include <sys/time.h>
#include <util/units.hpp>

namespace x86
{
  struct Clocks {
    static void init();
    static util::KHz  get_khz();
  };
}
