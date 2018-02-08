#include "common.hpp"
#include <stdlib.h>

extern "C"
int syscall_SYS_tkill(int tid, int sig) {
  STRACE("tkill(%i, %i)", tid, sig);
  #ifdef INCLUDEOS_SINGLE_THREADED
  exit(sig);
  #else
  #error "tkill not implemented for threaded IncludeOS"
  #endif
}
