#include "common.hpp"

extern "C"
long syscall_SYS_munmap() {
  STUB("munmap");
  return 0;
}
