#include "common.hpp"
#include <sys/stat.h>
#include <fcntl.h>

static long sys_openat(int /*dirfd*/, const char* /*path*/, int /*flags*/, mode_t)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_openat(int dirfd, const char *pathname, int flags, mode_t mode) {
  return strace(sys_openat, "openat", dirfd, pathname, flags, mode);
}
