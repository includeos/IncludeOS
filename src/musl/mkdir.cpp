#include "common.hpp"
#include <sys/stat.h>

static long sys_mkdir(const char* /*path*/, mode_t /*mode*/) {
  // currently makes no sense, especially since we're read-only
  return -EROFS;
}

extern "C"
long syscall_SYS_mkdir(const char *path, mode_t mode) {
  return strace(sys_mkdir, "mkdir", path, mode);
}
