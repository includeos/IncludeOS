#include <os>
#include <errno.h>
#include <musl/common.hpp>
#include "cpu.hpp"

extern "C"
long sys_set_thread_area(struct user_desc *u_info)
{
  set_tpidr(u_info); // This probably still does not work
  return 0;
}

extern "C"
long syscall_SYS_set_thread_area(struct user_desc *u_desc) {
    return strace(sys_set_thread_area, "set_thread_area", u_desc);
}

extern "C"
long sys_arch_prctl(int code, uintptr_t ptr) {
    os::panic("This should not happen!");
    return -ENOSYS;
}

extern "C"
long syscall_SYS_arch_prctl(int code, uintptr_t ptr) {
    return strace(sys_arch_prctl, "arch_prctl", code, ptr);
}