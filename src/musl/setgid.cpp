#include "common.hpp"
#include <sys/types.h>
#include <unistd.h>

long sys_setgid(gid_t /*gid*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_setgid(gid_t gid)
{
  return strace(sys_setgid, "setgid", gid);
}
