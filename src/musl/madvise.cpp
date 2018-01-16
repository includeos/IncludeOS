#include "common.hpp"

extern "C"
long syscall_SYS_madvise() {
  STUB("madvise");
  return 0;
}
