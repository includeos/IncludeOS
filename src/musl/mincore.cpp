#include "common.hpp"

extern "C"
long syscall_SYS_mincore() {
  STUB("mincore");
  return 0;
}
