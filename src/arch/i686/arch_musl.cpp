#include <os>
#include <errno.h>
#include <musl/common.hpp>

extern "C"
long sys_set_thread_area(struct user_desc *u_info) {
    os::panic("This should not happen!");
    return -ENOSYS;
}

extern "C"
long syscall_SYS_set_thread_area(struct user_desc *u_info)
{
    return strace(sys_set_thread_area, "set_thread_area", u_info);
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