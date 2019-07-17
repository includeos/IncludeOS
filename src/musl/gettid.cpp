#include "common.hpp"
#include <kernel/threads.hpp>

static long sys_gettid() {
  return kernel::get_tid();
}

extern "C"
long syscall_SYS_gettid() {
  return strace(sys_gettid, "gettid");
}
