#include "common.hpp"

extern "C"
void __kfree(void* addr, size_t length);

extern "C"
int syscall_SYS_munmap(void *addr, size_t length)
{
  STUB("munmap");
  __kfree(addr, length);
  return 0;
}
