#pragma once
#include <cstdint>
#include <sys/time.h>
#include <util/units.hpp>

struct KVM_clock
{
  static void init();
  static uint64_t system_time();
  static timespec wall_clock();
  static util::KHz      get_tsc_khz();
  static void deactivate();
};
