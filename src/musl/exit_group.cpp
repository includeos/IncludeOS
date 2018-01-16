#include "common.hpp"

extern "C"
long syscall_SYS_exit_group() {
  STUB("exit_group");
  return 0;
}
