#include "common.hpp"
#include <sys/types.h>
#include <unistd.h>

long sys_setuid(uid_t /*uid*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_setuid(uid_t uid)
{
  return strace(sys_setuid, "setuid", uid);
}
