#include "common.hpp"
#include <sys/stat.h>

long sys_statx(int /*dirfd*/, const char* /*pathname*/, int /*flags*/,
      unsigned int /*mask*/, struct statx* /*statxbuf*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_statx(int dirfd, const char *pathname, int flags,
      unsigned int mask, struct statx *statxbuf) {
  return strace(sys_statx, "statx", dirfd, pathname, flags, mask, statxbuf);
}

