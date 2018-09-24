#include "common.hpp"
#include <unistd.h>
#include <sys/types.h>

static long sys_ftruncate(int /*fd*/, off_t /*length*/)
{
  return -EROFS;
}

extern "C"
long syscall_SYS_ftruncate(int fd, off_t length)
{
  return strace(sys_ftruncate, "ftruncate", fd, length);
}
