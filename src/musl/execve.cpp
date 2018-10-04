#include "common.hpp"
#include <sys/types.h>
#include <unistd.h>

static long sys_execve(const char*, char *const[], char *const[])
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_execve(const char *filename, char *const argv[],
                     char *const envp[])
{
  return strace(sys_execve, "execve", filename, argv, envp);
}
