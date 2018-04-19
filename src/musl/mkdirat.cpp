#include "common.hpp"
#include <sys/stat.h>
#include <fcntl.h>

static long sys_mkdirat(int /*fd*/, const char* /*path*/, mode_t /*mode*/) {
  // currently makes no sense, especially since we're read-only
  return -EROFS;
}

extern "C"
long syscall_SYS_mkdirat(int fd, const char *path, mode_t mode) {
  return strace(sys_mkdirat, "mkdirat", fd, path, mode);
}
