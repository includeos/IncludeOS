#include "stub.hpp"
#include <poll.h>
#include <signal.h>


static long sys_poll(struct pollfd *fds, nfds_t nfds, int /*timeout*/)
{
  for (nfds_t i = 0; i < nfds; i++)
  {
    fds[i].revents = fds[i].events;
  }
  return nfds;
}
static long sys_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec * /*timeout_ts*/, const sigset_t * /*sigmask*/)
{
  for (nfds_t i = 0; i < nfds; i++)
  {
    fds[i].revents = fds[i].events;
  }
  return nfds;
}
extern "C"
long syscall_SYS_poll(struct pollfd *fds, nfds_t nfds, int timeout)
{
  return stubtrace(sys_poll, "poll", fds, nfds, timeout);
}

extern "C"
int syscall_SYS_ppoll(struct pollfd *fds, nfds_t nfds, const struct timespec *timeout_ts, const sigset_t *sigmask)
{
	return stubtrace(sys_ppoll, "ppoll", fds, nfds, timeout_ts,sigmask);
}
