#include "common.hpp"
#include <kernel/threads.hpp>

static long sys_set_tid_address(int* ctid) {
  auto* kthread = kernel::get_thread();
  kthread->clear_tid = ctid;
  return kthread->tid;
}

extern "C"
long syscall_SYS_set_tid_address(int* tidptr) {
  return strace(sys_set_tid_address, "set_tid_address", tidptr);
}
