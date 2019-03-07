#include "common.hpp"
#include <sys/stat.h>

static long sys_mknod(const char* /*pathname*/, mode_t /*mode*/, dev_t /*dev*/)
{
  // currently makes no sense, especially since we're read-only
  return -EROFS;
}

extern "C"
long syscall_SYS_mknod(const char *pathname, mode_t mode, dev_t dev) {
  return strace(sys_mknod, "mknod", pathname, mode, dev);
}
