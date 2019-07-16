#include "common.hpp"
#include <arch.hpp>
#include <string>
#include <os>
#include <kernel/threads.hpp>

// We can't use the usual "info", as printf isn't available after call to exit
#define SYSINFO(TEXT, ...) kprintf("%13s ] " TEXT "\n", "[ Kernel", ##__VA_ARGS__)

__attribute__((noreturn))
static long sys_exit(int status)
{
  auto* t = kernel::get_thread();
  if (t->tid == 0) {
    const std::string msg = "Service exited with status " + std::to_string(status) + "\n";
    os::print(msg.data(), msg.size());
    __arch_poweroff();
  }
  else {
    // exit from a thread
#ifdef THREADS_DEBUG
    int64_t ptid = -1;
    if (t->parent != nullptr) ptid = t->parent->tid;
    THPRINT("thread_exit tid=%ld  parent=%p  ptid: %ld\n",
            t->tid, t->parent, ptid);
#endif
    t->exit();
  }
  __builtin_unreachable();
}

extern "C"
void syscall_SYS_exit_group(int status)
{
  auto* t = kernel::get_thread();
  SYSINFO("Service exiting with status %d (thread %ld)\n", status, t->tid);
  kernel::default_exit();
  __builtin_unreachable();
}

extern "C"
void syscall_SYS_exit(int status) {
  strace(sys_exit, "exit", status);
}
