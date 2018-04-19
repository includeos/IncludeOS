#include "common.hpp"

extern "C"
long syscall_SYS_set_robust_list() {
  STUB("set_robust_list");
  return 0;
}
