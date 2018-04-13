#include "stub.hpp"
#include <signal.h>

static int sys_rt_sigprocmask (int /*how*/, const sigset_t*, sigset_t* /*old*/){
  return 0;
}

static int sys_sigmask(int /*signum*/)
{
  return 0;
}

extern const bool __strace;

extern "C"
int syscall_SYS_rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {

  if constexpr (__strace)
  {
    const char* howstr = nullptr;

    switch(how) {
    case SIG_BLOCK:
      howstr = "BLOCK";
      break;

    case SIG_UNBLOCK:
      howstr = "UNBLOCK";
      break;

    case SIG_SETMASK:
      howstr = "SETMASK";
      break;
    }
    auto ret = sys_rt_sigprocmask(how, set, oldset);
    stubtrace_print("sys_rt_sigprocmask", howstr, set, oldset);
    return ret;
  }
  return sys_rt_sigprocmask(how, set, oldset);
}

extern "C"
long syscall_SYS_sigmask(int signum) {
  return stubtrace(sys_sigmask, "sigmask", signum);
}
