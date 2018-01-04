#pragma once
#include <cstdint>
#include <sys/time.h>
#include <hertz>

struct KVM_clock
{
  static void init();
  static uint64_t system_time();
  static timespec wall_clock();
  static KHz      get_tsc_khz();
};
