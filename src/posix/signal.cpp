#include <signal.h>
#include <errno.h>
#include <cstdio>

int sigaction(int signum,
              const struct sigaction* act,
              struct sigaction* oldact)
{
  if (signum == SIGKILL || signum == SIGSTOP) {
    errno = EINVAL;
    return -1;
  }
  printf("sigaction(%d, %p, %p)\n", signum, act, oldact);
  *oldact = *act;
  return 0;
}
