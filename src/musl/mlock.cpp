#include "common.hpp"
#include <kernel/memory.hpp>

static long sys_mlock(const void* addr, size_t len)
{
  return -ENOSYS;
}
static long sys_munlock(const void* addr, size_t len)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_mlock(const void *addr, size_t len)
{
  return strace(sys_mlock, "mlock", addr, len);
}
extern "C"
long syscall_SYS_munlock(const void *addr, size_t len)
{
  return strace(sys_munlock, "munlock", addr, len);
}
