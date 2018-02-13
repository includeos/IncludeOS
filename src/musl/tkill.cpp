#include "common.hpp"
#include <stdlib.h>

static int sys_tkill(int tid, int sig) {
  #ifdef INCLUDEOS_SINGLE_THREADED
  exit(sig);
  #else
  #error "tkill not implemented for threaded IncludeOS"
  #endif
}

extern "C"
int syscall_SYS_tkill(int tid, int sig) {
  return strace(sys_tkill, "tkill", tid, sig);
}
