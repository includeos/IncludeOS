#include "common.hpp"
#include <arch.hpp>
#include <string>
#include <os>

__attribute__((noreturn))
static long sys_exit(int status)
{
  const std::string msg = "Service exited with status " + std::to_string(status) + "\n";
  os::print(msg.data(), msg.size());
  __arch_poweroff();
  __builtin_unreachable();
}

extern "C"
void syscall_SYS_exit(int status) {
  strace(sys_exit, "exit", status);
}
