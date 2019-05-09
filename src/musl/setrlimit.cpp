#include "common.hpp"

long sys_setrlimit(int /*resource*/, const struct rlimit* /*rlim*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_setrlimit(int resource,
               const struct rlimit* rlim)
{
  return strace(sys_setrlimit, "setrlimit", resource, rlim);
}
