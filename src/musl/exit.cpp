#include "common.hpp"
#include <arch.hpp>
#include <string>
#include <os>
#include <kernel/threads.hpp>

__attribute__((noreturn))
static long sys_exit(int status)
{
  auto* t = kernel::get_thread();
  if (t == 0) {
    const std::string msg = "Service exited with status " + std::to_string(status) + "\n";
    os::print(msg.data(), msg.size());
    __arch_poweroff();
  }
  else {
    // exit from a thread
    kernel::thread_exit();
  }
  __builtin_unreachable();
}

extern "C"
void syscall_SYS_exit(int status) {
  strace(sys_exit, "exit", status);
}
