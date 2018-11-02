#include "stub.hpp"

static long sys_sched_yield()
{
  return 0;
}

extern "C"
long syscall_SYS_sched_yield() {
  return stubtrace(sys_sched_yield, "sched_yield");
}
