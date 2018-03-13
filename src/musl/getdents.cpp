#include "common.hpp"

static int sys_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count) {
  // currently makes no sense, especially since we're read-only
  errno = EROFS;
  return -1;
}

extern "C"
int syscall_SYS_getdents(unsigned int fd, struct linux_dirent *dirp, unsigned int count) {
  return strace(sys_getdents, "getdents", fd, dirp, count);
}
