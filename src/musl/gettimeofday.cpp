#include "common.hpp"

extern "C"
long syscall_SYS_gettimeofday() {
  STUB("gettimeofday");
  return 0;
}
