#include "common.hpp"

extern "C" void kfree(void* addr, size_t length);

extern "C"
int syscall_SYS_munmap(void *addr, size_t length)
{
  STUB("munmap");
  kfree(addr, length);
  return 0;
}
