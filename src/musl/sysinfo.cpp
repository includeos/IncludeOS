#include "common.hpp"

extern "C"
long syscall_SYS_sysinfo() {
  STUB("sysinfo");
  return 0;
}
