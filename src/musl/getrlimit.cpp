#include "common.hpp"

extern "C"
long syscall_SYS_getrlimit() {
  STUB("getrlimit");
  return 0;
}
