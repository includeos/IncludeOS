#include "common.hpp"
#include <sys/stat.h>
#include <fcntl.h>

static long sys_fchmodat2(int /*fd*/, const char* /*path*/, mode_t, int /*flag*/)
{
  // currently makes no sense, especially since we're read-only
  return -EROFS;
}

extern "C"
long syscall_SYS_fchmodat2(int fd, const char *path, mode_t mode, int flag) {
  return strace(sys_fchmodat2, "fchmodat2", fd, path, mode, flag);
}
