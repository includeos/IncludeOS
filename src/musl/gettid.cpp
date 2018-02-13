#include "common.hpp"

static long sys_gettid() {
#ifdef INCLUDEOS_SINGLE_THREADED
  return 0;
#else
#error "gettid not implemented for threaded IncludeOS"
#endif
}

extern "C"
long syscall_SYS_gettid() {
  return strace(sys_gettid, "gettid");
}
