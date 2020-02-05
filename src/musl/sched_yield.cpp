#include "stub.hpp"
#include <kernel/threads.hpp>

static long sys_sched_yield()
{
    THPRINT("sched_yield() called on thread %ld\n", kernel::get_tid());
    __thread_yield();
    return 0;
}

extern "C"
long syscall_SYS_sched_yield() {
  return strace(sys_sched_yield, "sched_yield");
}

extern "C"
long syscall_SYS_sched_setscheduler(pid_t /*pid*/, int /*policy*/,
                          const struct sched_param* /*param*/)
{
  return 0;
}
