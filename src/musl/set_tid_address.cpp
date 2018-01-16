#include "common.hpp"

extern "C"
long syscall_SYS_set_tid_address() {
  STUB("set_tid_address");
  return 0;
}
