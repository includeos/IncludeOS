#include "common.hpp"

static long sys_getuid() {
  return 0;
}

extern "C"
long syscall_SYS_getuid() {
  return strace(sys_getuid, "getuid");
}
