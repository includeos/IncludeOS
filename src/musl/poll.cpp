#include "stub.hpp"
#include <poll.h>

static long sys_poll(struct pollfd *fds, nfds_t nfds, int /*timeout*/)
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
