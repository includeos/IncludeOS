#include "common.hpp"
#include <unistd.h>

static long sys_fchown(int /*fd*/, uid_t /*owner*/, gid_t /*group*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_fchown(int fd, uid_t owner, gid_t group)
{
  return strace(sys_fchown, "fchown", fd, owner, group);
}
