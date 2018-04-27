#include "common.hpp"

extern "C"
long syscall_SYS_msync() {
  STUB("msync");
  return 0;
}
