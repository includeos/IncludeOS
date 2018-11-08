#include "common.hpp"
#include <kernel/memory.hpp>

static long sys_mprotect(void* /*addr*/, size_t /*len*/, int /*prot*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_mprotect(void *addr, size_t len, int prot)
{
  return strace(sys_mprotect, "mprotect", addr, len, prot);
}
