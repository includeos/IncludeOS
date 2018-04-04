#include "common.hpp"
#include "stub.hpp"
#include <errno.h>

extern "C"
long socketcall_socket()
{
  return -ENOSYS;
}
extern "C"
long socketcall_connect()
{
  return -ENOSYS;
}
extern "C"
long socketcall_shutdown()
{
  return -ENOSYS;
}

static long sock_sendmsg(int /*sockfd*/, const struct msghdr */*msg*/, int /*flags*/)
{
  return -ENOSYS;
}

extern "C"
long socketcall_sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
  return stubtrace(sock_sendmsg, "sendmsg", sockfd, msg, flags);
}

