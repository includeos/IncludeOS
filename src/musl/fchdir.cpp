#include "common.hpp"
#include <unistd.h>

static long sys_fchdir(int /*fd*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_fchdir(int fd)
{
  return strace(sys_fchdir, "fchown", fd);
}
