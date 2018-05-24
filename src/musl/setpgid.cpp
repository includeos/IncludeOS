#include "common.hpp"
#include <sys/types.h>
#include <unistd.h>

long sys_setpgid(pid_t /*pid*/, gid_t /*gid*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_setpgid(pid_t pid, gid_t gid)
{
  return strace(sys_setpgid, "setpgid", pid, gid);
}
