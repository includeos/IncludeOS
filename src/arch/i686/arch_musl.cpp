#include <os>
#include <errno.h>

extern "C"
long syscall_SYS_set_thread_area(struct user_desc *u_info)
{
  os::panic("Libc not supported for 32 bit currently!");
  return -ENOSYS;
}

extern "C"
long syscall_SYS_arch_prctl(int code, uintptr_t ptr) {
  os::panic("Libc not supported for 32 bit currently!");
  return -ENOSYS;
}
