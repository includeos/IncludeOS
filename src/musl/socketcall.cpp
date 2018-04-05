#include "common.hpp"
#include <errno.h>
#include <sys/socket.h>

#include <posix/fd_map.hpp>

static long sock_socket(int domain, int type, int protocol)
{
  // disallow strange domains, like ALG
  if (UNLIKELY(domain < 0 || domain > AF_INET6))
    return -EAFNOSUPPORT;
  // disallow RAW etc
  if (UNLIKELY(type < 0 || type > SOCK_DGRAM))
    return -EINVAL;
  // we are purposefully ignoring the protocol argument
  if (UNLIKELY(protocol < 0))
    return -EPROTONOSUPPORT;

  /*return [](const int type)->int{
    switch(type)
    {
      case SOCK_STREAM:
        return FD_map::_open<TCP_FD>().get_id();
      case SOCK_DGRAM:
        return FD_map::_open<UDP_FD>().get_id();
      default:
        errno = EINVAL;
        return -1;
    }
  }(type);*/
  return -EINVAL;
}

static long sock_connect(int sockfd, const struct sockaddr *addr,
                         socklen_t addrlen)
{
  try {
    auto& fildes = FD_map::_get(sockfd);
    return fildes.connect(addr, addrlen);
  }
  catch(const FD_not_found&) {
    return -EBADF;
  }
}

static long sock_bind(int sockfd, const struct sockaddr *addr,
                      socklen_t addrlen)
{
  try {
    auto& fildes = FD_map::_get(sockfd);
    return fildes.connect(addr, addrlen);
  }
  catch(const FD_not_found&) {
    return -EBADF;
  }
}

static long sock_sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
  try {
    auto& fildes = FD_map::_get(sockfd);
    return fildes.sendmsg(msg, flags);
  }
  catch(const FD_not_found&) {
    return -EBADF;
  }
}

static ssize_t sock_sendto(int sockfd, const void *buf, size_t len, int flags,
                           const struct sockaddr *dest_addr, socklen_t addrlen)
{
  try {
    auto& fildes = FD_map::_get(sockfd);
    return fildes.sendto(buf, len, flags, dest_addr, addrlen);
  }
  catch(const FD_not_found&) {
    return -EBADF;
  }
}

static ssize_t sock_recvfrom(int sockfd, void *buf, size_t len, int flags,
                             struct sockaddr *src_addr, socklen_t *addrlen)
{
  try {
    auto& fildes = FD_map::_get(sockfd);
    return fildes.recvfrom(buf, len, flags, src_addr, addrlen);
  }
  catch(const FD_not_found&) {
    return -EBADF;
  }
}

extern "C"
long socketcall_socket(int domain, int type, int protocol)
{
  return strace(sock_socket, "socket", domain, type, protocol);
}

extern "C"
long socketcall_connect(int sockfd, const struct sockaddr *addr,
                        socklen_t addrlen)
{
  return strace(sock_connect, "connect", sockfd, addr, addrlen);
}

extern "C"
long socketcall_bind(int sockfd, const struct sockaddr *addr,
                     socklen_t addrlen)
{
  return strace(sock_bind, "bind", sockfd, addr, addrlen);
}

extern "C"
long socketcall_sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
  return strace(sock_sendmsg, "sendmsg", sockfd, msg, flags);
}

extern "C"
ssize_t socketcall_sendto(int sockfd, const void *buf, size_t len, int flags,
                          const struct sockaddr *dest_addr, socklen_t addrlen)
{
  return strace(sock_sendto, "sendto", sockfd, buf, len, flags, dest_addr, addrlen);
}

extern "C"
ssize_t socketcall_recvfrom(int sockfd, void *buf, size_t len, int flags,
                            struct sockaddr *src_addr, socklen_t *addrlen)
{
  return strace(sock_recvfrom, "recvfrom", sockfd, buf, len, flags, src_addr, addrlen);
}

extern "C"
long socketcall_shutdown()
{
  return -ENOSYS;
}
