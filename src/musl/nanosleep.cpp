#include "common.hpp"
#include <time.h>

static long sys_nanosleep(const struct timespec */*req*/, struct timespec */*rem*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_nanosleep(const struct timespec *req, struct timespec *rem)
{
  return strace(sys_nanosleep, "nanosleep", req, rem);
}

extern "C"
long syscall_SYS_clock_nanosleep(clockid_t, int,
        const struct timespec *req, struct timespec *rem)
{
  return strace(sys_nanosleep, "clock_nanosleep", req, rem);
}

extern "C"
long syscall_SYS_clock_nanosleep_time64(clockid_t, int,
        const struct timespec *req, struct timespec *rem)
{
  return strace(sys_nanosleep, "clock_nanosleep_time64", req, rem);
}
