#include "common.hpp"
#include <sys/types.h>
#include <unistd.h>

long sys_setsid()
{
  return 0;
}

extern "C"
long syscall_SYS_setsid()
{
  return strace(sys_setsid, "setsid");
}
