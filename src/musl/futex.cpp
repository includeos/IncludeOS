#include "common.hpp"

extern "C"
long syscall_SYS_futex() {
  STUB("Futex");
  return 0;
}
