#include "stub.hpp"

static long sys_gettid() {
#ifndef INCLUDEOS_SINGLE_THREADED
#warning "gettid not implemented for threaded IncludeOS"
#endif
  return 1;
}

extern "C"
long syscall_SYS_gettid() {
  return stubtrace(sys_gettid, "gettid");
}
