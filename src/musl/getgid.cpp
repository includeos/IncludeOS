#include "common.hpp"
#include <sys/types.h>

static long sys_getgid()
{
  return 0;
}

extern "C"
long syscall_SYS_getgid()
{
  return strace(sys_getgid, "getgid");
}
