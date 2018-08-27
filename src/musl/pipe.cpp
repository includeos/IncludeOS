#include "common.hpp"

static long sys_pipe([[maybe_unused]]int pipefd[2])
{
  return -ENOSYS;
}

static long sys_pipe2([[maybe_unused]]int pipefd[2], [[maybe_unused]]int flags)
{
  return -ENOSYS;
}

extern "C" {
long syscall_SYS_pipe(int pipefd[2]) {
  return strace(sys_pipe, "pipe", pipefd);
}

long syscall_SYS_pipe2(int pipefd[2], int flags) {
  return strace(sys_pipe2, "pipe2", pipefd, flags);
}
} // extern "C"
