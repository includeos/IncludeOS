#include "common.hpp"

extern "C"
long syscall_SYS_exit() {
  STUB("exit");
  return 0;
}
