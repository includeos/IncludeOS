#include "common.hpp"

extern "C"
long syscall_SYS_close() {
  STUB("close");
  return 0;
}
