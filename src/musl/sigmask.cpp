#include "common.hpp"
#include <signal.h>

extern "C" {
long syscall_SYS_sigmask() {
  STUB("sigmask");
  return 0;
}

long syscall_SYS_rt_sigprocmask(int how, const sigset_t *set, sigset_t *oldset) {

char* howstr = nullptr;

switch(how) {

 case (SIG_BLOCK):
howstr="BLOCK";
break;

 case (SIG_UNBLOCK):
howstr="UNBLOCK";
break;

 case (SIG_SETMASK):
howstr="SETMASK";
break;

}

STRACE("rt_sigprocmask how=%i (%s), set=%p, oldset=%p\n",
         how, howstr, set, oldset);

return 0;
}
} // extern "C"
