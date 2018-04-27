#include "common.hpp"

int sys_getpid() {
  return 1;
}

extern "C"
long syscall_SYS_getpid() {
  return strace(sys_getpid, "getpid");
}
