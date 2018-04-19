#include "common.hpp"

extern "C"
long syscall_SYS_sched_yield() {
  STUB("sched_yield");
  return 0;
}
