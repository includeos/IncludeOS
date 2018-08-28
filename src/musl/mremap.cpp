#include "common.hpp"

static long sys_mremap(void */*old_address*/, size_t /*old_size*/,
                       size_t /*new_size*/, int /*flags*/, void */*new_address*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_mremap(void *old_address, size_t old_size,
                        size_t new_size, int flags, void *new_address)
{
  return strace(sys_mremap, "mremap", old_address, old_size, new_size, flags, new_address);
}
