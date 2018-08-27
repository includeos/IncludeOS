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
