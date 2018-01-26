#include "common.hpp"

extern "C"
long syscall_SYS_read() {
  STUB("read");
  return 0;
}
