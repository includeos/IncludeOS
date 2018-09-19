#include "common.hpp"
#include <sys/stat.h>

static long sys_fchmod(int /*fd*/, mode_t /*mode*/) {
  // currently makes no sense, especially since we're read-only
  return -EROFS;
}

extern "C"
long syscall_SYS_fchmod(int fildes, mode_t mode) {
  return strace(sys_fchmod, "fchmod", fildes, mode);
}
