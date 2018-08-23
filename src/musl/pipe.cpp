#include "stub.hpp"

static long sys_pipe()
{
  return 0;
}

static long sys_pipe2()
{
  return 0;
}

extern "C" {
long syscall_SYS_pipe() {
  return stubtrace(sys_pipe, "pipe");
}

long syscall_SYS_pipe2() {
  return stubtrace(sys_pipe2, "pipe2");
}
} // extern "C"
