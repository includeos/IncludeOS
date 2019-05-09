

#include "cpu.h"


extern "C"
long syscall_SYS_set_thread_area(struct user_desc *u_info)
{
  set_tpidr(u_info);
  /*
  if (UNLIKELY(!u_info)) return -EINVAL;
#ifdef __x86_64__
  x86::CPU::set_fs(u_info);
#else
  x86::CPU::set_gs(u_info);
#endif*/
  return 0;
}
