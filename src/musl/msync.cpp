#include "common.hpp"

static long sys_msync(void */*addr*/, size_t /*length*/, int /*flags*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_msync(void *addr, size_t length, int flags)
{
  return strace(sys_msync, "msync", addr, length, flags);
}
