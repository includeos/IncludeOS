#include "common.hpp"
#include <arch.hpp>
#include <string>
#include <os>

static long sys_exit_group(int status) {
  const std::string msg = "(Exit group - terminating threads) Service exited with status "
    + std::to_string(status) + "\n";
  OS::print(msg.data(), msg.size());
  __arch_poweroff();
  return status;
}

extern "C" __attribute__((noreturn))
long syscall_SYS_exit_group(int status) {
  strace(sys_exit_group, "exit_group", status);
}
