#include "common.hpp"

static long sys_clock_gettime(clockid_t clk_id, struct timespec* tp)
{
  if (clk_id == CLOCK_REALTIME)
  {
    *tp = __arch_wall_clock();
    return 0;
  }
  else if (clk_id == CLOCK_MONOTONIC || clk_id == CLOCK_MONOTONIC_RAW)
  {
    uint64_t ts = __arch_system_time();
    tp->tv_sec  = ts / 1000000000ull;
    tp->tv_nsec = ts % 1000000000ull;
    return 0;
  }
  return -EINVAL;
}

extern "C"
long syscall_SYS_clock_gettime(clockid_t clk_id, struct timespec* tp) {
  return strace(sys_clock_gettime, "clock_gettime", clk_id, tp);
}
