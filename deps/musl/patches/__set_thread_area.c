#include "syscall.h"

#define ARCH_SET_FS 0x1002

int __set_thread_area(void *p)
{
    return __syscall(SYS_arch_prctl, ARCH_SET_FS, p);
}
