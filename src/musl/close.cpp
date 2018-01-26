#include "common.hpp"

extern "C"
int syscall_SYS_close(int fd) {
  STRACE("close fd=%i\n", fd);

  if (!fd) return -1;

  return 0;
}
