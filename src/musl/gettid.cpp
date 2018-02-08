#include "common.hpp"

extern "C"
long syscall_SYS_gettid() {
  STRACE("gettid");
  #ifdef INCLUDEOS_SINGLE_THREADED
  return 0;
  #else
  #error "gettid not implemented for threaded IncludeOS"
  #endif
}
