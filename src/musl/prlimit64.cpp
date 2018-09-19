#include "common.hpp"
#include <sys/resource.h>

static long sys_prlimit64(pid_t /*pid*/, int /*resource*/,
                          const struct rlimit */*new_limit*/,
                          struct rlimit */*old_limit*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_prlimit64(pid_t pid, int resource,
                           const struct rlimit *new_limit,
                           struct rlimit *old_limit)
{
  return strace(sys_prlimit64, "prlimit64", pid, resource, new_limit, old_limit);
}
