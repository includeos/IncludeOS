#include "common.hpp"

#warning stub

#ifdef INCLUDEOS_SINGLE_THREADED
struct {
  int tid = 1;
  int* set_child_tid = nullptr;
  int* clear_child_tid = nullptr;
} __main_thread__;
#else
#warning syscall set_tid_address is not yet thread safe
#endif

static long sys_set_tid_address(int* tidptr) {
  __main_thread__.clear_child_tid = tidptr;
  return __main_thread__.tid;
}

extern "C"
long syscall_SYS_set_tid_address(int* tidptr) {
  return strace(sys_set_tid_address, "set_tid_address", tidptr);
}
