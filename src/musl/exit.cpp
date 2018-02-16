#include "common.hpp"
#include <arch.hpp>
#include <string>
#include <os>

static int sys_exit(int status){
  const std::string msg = "Service exited with status " + std::to_string(status) + "\n";
  OS::print(msg.data(), msg.size());
  __arch_poweroff();
}

extern "C" __attribute__((noreturn))
void syscall_SYS_exit(int status) {
  strace(sys_exit, "exit", status);
}
