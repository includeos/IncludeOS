#include "common.hpp"
#include <os>

extern "C"
long syscall_SYS_exit() {
  STUB("exit");
  panic("Exiting\n");
  return 0;
}
