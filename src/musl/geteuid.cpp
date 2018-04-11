#include "common.hpp"
#include <unistd.h>
#include <sys/types.h>

static long sys_geteuid()
{
  return 0;
}

extern "C"
long syscall_SYS_geteuid()
{
  return strace(sys_geteuid, "geteuid");
}
