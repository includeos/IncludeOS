#include "common.hpp"
#include <unistd.h>

static long sys_fsync(int /*fd*/)
{
  return -EROFS;
}

extern "C"
long syscall_SYS_fsync(int fd)
{
  return strace(sys_fsync, "fsync", fd);
}
