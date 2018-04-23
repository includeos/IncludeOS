#include "common.hpp"

extern "C"
long syscall_SYS_nanosleep() {
  STUB("nanosleep");
  return 0;
}
