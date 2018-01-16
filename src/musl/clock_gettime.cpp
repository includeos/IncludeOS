#include "common.hpp"

extern "C"
long syscall_SYS_clock_gettime() {
  STUB("clock_gettime");
  return 0;
}
