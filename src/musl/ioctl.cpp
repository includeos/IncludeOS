#include "common.hpp"

extern "C"
long syscall_SYS_ioctl() {
  STUB("ioctl");
  return 0;
}
