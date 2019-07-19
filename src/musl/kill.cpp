#include "common.hpp"
#include <sys/types.h>
#include <kernel/threads.hpp>

long sys_kill(pid_t /*pid*/, int /*sig*/) {
  os::panic("KILL called");
}

long sys_tkill(int tid, int /*sig*/)
{
    if (tid == 0) {
        os::panic("TKILL on main thread");
    }

    auto* thread = kernel::get_thread(tid);
    THPRINT("TKILL on tid=%d where thread=%p\n", tid, thread);
    if (thread != nullptr) {
        thread->exit();
        return 0;
    }
    return -EINVAL;
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
