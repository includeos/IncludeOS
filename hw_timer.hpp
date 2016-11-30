#pragma once

#include <hw/cpu.hpp>
#include <cstdint>
#include <cstdio>
#include <string>

struct HW_timer
{
  HW_timer(const std::string& str) {
    context = str;
    printf("HW timer starting for %s\n", context.c_str());
    time    = hw::CPU::rdtsc();
  }
  ~HW_timer() {
    auto diff = hw::CPU::rdtsc() - time;

    using namespace std::chrono;
    double  div  = OS::cpu_freq().count() * 1000000.0;
    int64_t time = diff / div * 1000;

    printf("HW timer for %s: %lld (%lld ms)\n", context.c_str(), diff, time);
  }
private:
  std::string context;
  int64_t     time;
};
