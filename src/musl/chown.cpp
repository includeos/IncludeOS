#include "common.hpp"
#include <unistd.h>

static long sys_chown(const char* /*path*/, uid_t /*owner*/, gid_t /*group*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_chown(const char *path, uid_t owner, gid_t group)
{
  return strace(sys_chown, "chown", path, owner, group);
}
