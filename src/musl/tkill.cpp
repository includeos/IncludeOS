#include "stub.hpp"
#include <stdlib.h>

static int sys_tkill(int tid, int sig) {

#ifndef INCLUDEOS_SINGLE_THREADED
#warning "tkill not implemented for threaded IncludeOS"
#endif

  exit(sig);
}

extern "C"
int syscall_SYS_tkill(int tid, int sig) {
  return stubtrace(sys_tkill, "tkill", tid, sig);
}
