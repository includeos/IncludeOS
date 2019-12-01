#include "common.hpp"
#include <kernel/mrspinny.hpp>

extern "C" void kfree(void* addr, size_t length);

static long sys_munmap(void *addr, size_t length)
{
  if(UNLIKELY(length == 0))
    return -EINVAL;

  scoped_spinlock { mr_spinny.memory };
  kfree(addr, length);
  return 0;
}

extern "C"
long syscall_SYS_munmap(void *addr, size_t length)
{
  return strace(sys_munmap, "munmap", addr, length);
}
