#include "common.hpp"

extern "C"
long syscall_SYS_sched_getaffinity() {
  STUB("sched_getaffinity");
  return 0;
}
