#include "common.hpp"
#include <sys/time.h>

static long sys_gettimeofday(struct timeval *tv, struct timezone *tz)
{
  timespec t  = __arch_wall_clock();
  tv->tv_sec  = t.tv_sec;
  tv->tv_usec = t.tv_nsec * 1000;
  if (tz != nullptr) {
    tz->tz_minuteswest = 0;
    tz->tz_dsttime     = 0; /* DST_NONE */
  }
  return 0;
}

extern "C"
long syscall_SYS_gettimeofday(struct timeval *tv, struct timezone *tz) {
  return strace(sys_gettimeofday, "gettimeofday", tv, tz);
}
