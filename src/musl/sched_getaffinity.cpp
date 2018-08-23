#include "stub.hpp"

static long sys_sched_getaffinity()
{
  return 0;
}

extern "C"
long syscall_SYS_sched_getaffinity() {
  return stubtrace(sys_sched_getaffinity, "sched_getaffinity");
}
