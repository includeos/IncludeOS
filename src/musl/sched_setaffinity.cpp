#include "common.hpp"
#include <sched.h>

static long sys_sched_setaffinity(pid_t /*pid*/, size_t /*cpusetsize*/,
                                  cpu_set_t */*mask*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_sched_setaffinity(pid_t pid, size_t cpusetsize,
                                   cpu_set_t *mask)
{
  return strace(sys_sched_setaffinity, "sched_setaffinity", pid, cpusetsize, mask);
}
