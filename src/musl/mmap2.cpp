#include "common.hpp"

extern "C"
long syscall_SYS_mmap2() {
  STUB("mmap2");
  return 0;
}
