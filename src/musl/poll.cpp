#include "common.hpp"
#include <poll.h>

extern "C"
long syscall_SYS_poll(struct pollfd *fds, nfds_t nfds, int /*timeout*/)
{
  STUB("poll");
  for (nfds_t i = 0; i < nfds; i++)
  {
    fds[i].revents = fds[i].events;
  }
  return nfds;
}
