#include "common.hpp"
#include <sys/types.h>
#include <unistd.h>

static long
sys_wait4(pid_t /*pid*/, int* /*wstatus*/, int /*options*/,
          struct rusage* /*rusage*/)
{
  return 0;
}

extern "C"
long syscall_SYS_wait4(pid_t pid, int *wstatus, int options,
                       struct rusage *rusage)
{
  return strace(sys_wait4, "wait4", pid, wstatus, options, rusage);
}
