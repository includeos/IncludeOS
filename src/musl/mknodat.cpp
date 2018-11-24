#include "common.hpp"
#include <sys/stat.h>

static long sys_mknodat(int /*dirfd*/, const char* /*path*/, mode_t, dev_t)
{
  // currently makes no sense, especially since we're read-only
  return -EROFS;
}

extern "C"
long syscall_SYS_mknodat(int dirfd, const char *pathname, mode_t mode, dev_t dev) {
  return strace(sys_mknodat, "mknodat", dirfd, pathname, mode, dev);
}
