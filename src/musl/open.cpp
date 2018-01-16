#include "common.hpp"

extern "C"
long syscall_SYS_open() {
  STUB("open");
  return 0;
}
