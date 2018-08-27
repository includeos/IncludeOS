#include "common.hpp"

static long sys_mincore([[maybe_unused]]void *addr,
                        [[maybe_unused]]size_t length,
                        [[maybe_unused]]unsigned char *vec)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_mincore(void *addr, size_t length, unsigned char *vec)
{
  return strace(sys_mincore, "mincore", addr, length, vec);
}
