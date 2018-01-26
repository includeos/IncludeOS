#include "common.hpp"

extern "C"
long syscall_SYS_getpid() {
  STRACE("getpid");
  return 0;
}
