#include "common.hpp"
#include <sys/types.h>
#include <kernel/threads.hpp>

long sys_kill(pid_t /*pid*/, int /*sig*/) {
  os::panic("KILL called");
}

long sys_tkill(int tid, int /*sig*/)
{
    char buffer[1024];
    snprintf(buffer, sizeof(buffer),
            "TKILL called on tid=%d, current tid=%d",
            tid, kernel::get_tid());
    os::panic(buffer);
}

long sys_tgkill(int /*tgid*/, int tid, int sig) {
  return sys_tkill(tid, sig);
}

extern "C"
long syscall_SYS_kill(pid_t pid, int sig) {
  return strace(sys_kill, "kill", pid, sig);
}

extern "C"
long syscall_SYS_tkill(int tid, int sig) {
  return strace(sys_tkill, "tkill", tid, sig);
}

extern "C"
long syscall_SYS_tgkill(int tgid, int tid, int sig)
{
  return strace(sys_tgkill, "tgkill", tgid, tid, sig);
}
