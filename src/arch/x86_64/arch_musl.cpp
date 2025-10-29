#include <os>
#include <errno.h>
#include <likely>
#include <arch/x86/cpu.hpp>

extern "C"
long syscall_SYS_set_thread_area(struct user_desc *u_desc) {
  os::panic("Setting thread pointer on 64 bit Intel/AMD should go via arch_prctl!");
  return -ENOSYS;
}

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

extern "C"
long syscall_SYS_arch_prctl(int code, uintptr_t ptr) {
  switch(code){
  case ARCH_SET_GS:
    if (UNLIKELY(!ptr)) return -EINVAL;
    x86::CPU::set_gs((void*)ptr);
    break;
  case ARCH_SET_FS:
    if (UNLIKELY(!ptr)) return -EINVAL;
    x86::CPU::set_fs((void*)ptr);
    break;
  case ARCH_GET_GS:
    os::panic("<arch_prctl> GET_GS called!\n");
  case ARCH_GET_FS:
    os::panic("<arch_prctl> GET_FS called!\n");
  }
  return 0;
}
