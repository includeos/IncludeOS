#include "common.hpp"

extern "C"
long syscall_SYS_poll() {
  STUB("poll");
  return 0;
}
