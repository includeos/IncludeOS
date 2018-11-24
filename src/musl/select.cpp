#include "common.hpp"
#include <sys/select.h>

long sys_select(int /*nfds*/,
                fd_set* /*readfds*/,
                fd_set* /*writefds*/,
                fd_set* /*exceptfds*/,
                struct timeval* /*timeout*/)
{
  return -ENOSYS;
}

extern "C"
long syscall_SYS_select(int nfds,
               fd_set* readfds,
               fd_set* writefds,
               fd_set* exceptfds,
               struct timeval* timeout)
{
  return strace(sys_select, "select", nfds, readfds, writefds, exceptfds, timeout);
}
