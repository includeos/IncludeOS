#include "common.hpp"

extern "C"
long syscall_SYS_prlimit64() {
  STUB("prlimit64");
  return 0;
}
