#pragma once
#include <cstdint>

struct KVM_clock
{
  static void init();
  static uint64_t system_time();
  static uint64_t wall_clock();
};
