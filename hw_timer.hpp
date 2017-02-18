/**
 * Master thesis
 * by Alf-Andre Walla 2016-2017
 * 
**/
#pragma once

#include <kernel/os.hpp>
#include <cstdint>
#include <cstdio>

struct HW_timer
{
  HW_timer(const char* ctx)
    : context(ctx), time(OS::cycles_since_boot()) {}
  ~HW_timer() {
    const auto   diff = OS::cycles_since_boot() - time;
    const double div  = OS::cpu_freq().count() * 1000.0;
    const double time = diff / div;

    printf("HW timer for %s: %lld (%.2f ms)\n", context, diff, time);
  }
private:
  const char* context;
  int64_t     time;
};
