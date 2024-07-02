#include "common.hpp"
#include <sched.h>

static long sys_sched_setscheduler(pid_t /*pid*/, int /*policy*/, const struct sched_param* /*param*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_sched_setscheduler(pid_t pid, int policy,
                                   const struct sched_param *param)
{
  return strace(sys_sched_setscheduler, "sched_setscheduler", pid, policy, param);
}
