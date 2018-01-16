#include "common.hpp"

extern "C"
long syscall_SYS_fstat() {
  STUB("fstat");
  return 0;
}
