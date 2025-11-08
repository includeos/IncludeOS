#include <os>
#include <errno.h>
#include "cpu.hpp"

extern "C"
long syscall_SYS_set_thread_area(struct user_desc *u_desc) {
  set_tpidr(u_info); // This probably still does not work
  return 0;
}

extern "C"
long syscall_SYS_arch_prctl(int code, uintptr_t ptr) {
  os::panic("Arch_prctl is specific to x86!");
  return -ENOSYS;
}
