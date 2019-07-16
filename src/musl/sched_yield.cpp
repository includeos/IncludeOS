#include "stub.hpp"
#include <kernel/threads.hpp>

extern "C" {
  void __thread_yield();
}

static long sys_sched_yield()
{
  __thread_yield();
  return 0;
}

extern "C"
long syscall_SYS_sched_yield() {
  return stubtrace(sys_sched_yield, "sched_yield");
}
