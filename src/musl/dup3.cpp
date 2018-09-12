#include "common.hpp"
#include <sys/types.h>
#include <unistd.h>

static long sys_dup(int /*oldfd*/)
{
  return -ENOSYS;
}
static long sys_dup2(int /*oldfd*/, int /*newfd*/)
{
  return -ENOSYS;
}
static long sys_dup3(int /*oldfd*/, int /*newfd*/, int /*flags*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_dup(int oldfd) {
  return strace(sys_dup, "dup", oldfd);
}

extern "C"
long syscall_SYS_dup2(int oldfd, int newfd) {
  return strace(sys_dup2, "dup2", oldfd, newfd);
}

extern "C"
long syscall_SYS_dup3(int oldfd, int newfd, int flags) {
  return strace(sys_dup3, "dup3", oldfd, newfd, flags);
}
