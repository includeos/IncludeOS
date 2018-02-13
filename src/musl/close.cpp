#include "common.hpp"

static int sys_close(int fd) {
  if (!fd) return -1;
  return 0;

};

extern "C"
int syscall_SYS_close(int fd) {
  return strace(sys_close, "close", fd);
}
