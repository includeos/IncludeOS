#include "common.hpp"

extern "C"
long syscall_SYS_write() {
  STUB("write");
  return 0;
}
