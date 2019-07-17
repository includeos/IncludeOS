#include "common.hpp"
#include <kernel/threads.hpp>

static long sys_set_tid_address(int* /*tidptr*/) {
  return kernel::get_tid();
}

extern "C"
long syscall_SYS_set_tid_address(int* tidptr) {
  return strace(sys_set_tid_address, "set_tid_address", tidptr);
}
