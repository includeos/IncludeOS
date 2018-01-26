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

extern "C"
long syscall_SYS_set_tid_address(int* tidptr) {
  STRACE("set_tid_address, %p\n", tidptr);
  if (tidptr) {
    kprintf("\t*tidptr: 0x%x\n", *tidptr);
  }
  __main_thread__.clear_child_tid = tidptr;

  return __main_thread__.tid;
}
