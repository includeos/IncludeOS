#include "common.hpp"
#include <sys/stat.h>

static int sys_chmod(const char *path, mode_t mode) {
  // currently makes no sense, especially since we're read-only
  errno = EROFS;
  return -1;
}

extern "C"
int syscall_SYS_chmod(const char *path, mode_t mode) {
  return strace(sys_chmod, "chmod", path, mode);
}
