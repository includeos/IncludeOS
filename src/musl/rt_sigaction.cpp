#include "common.hpp"

static int sys_sigaction(int signum,
                         const struct sigaction* act,
                         const struct sigaction* oldact)
{
  (void) signum;
  (void) act;
  (void) oldact;
  return -ENOSYS;
}

extern "C"
int syscall_SYS_rt_sigaction(int signum,
                             const struct sigaction* act,
                             const struct sigaction* oldact)
{
  return strace(sys_sigaction, "rt_sigaction", signum, act, oldact);
}
