#include "common.hpp"
#include <errno.h>
#include <sys/socket.h>

#include <posix/fd_map.hpp>
#include <posix/tcp_fd.hpp>
#include <posix/udp_fd.hpp>
#include <posix/unix_fd.hpp>

static long sock_socket(int domain, int type, int protocol)
{
  if(domain == AF_UNIX)
    return FD_map::_open<Unix_FD>(type).get_id();
  // currently only support for AF_INET (IPv4, no local/unix or IP6)
  if (UNLIKELY(domain != AF_INET))
    return -EAFNOSUPPORT;
  // disallow RAW etc
  if (UNLIKELY(type < 0 || type > SOCK_DGRAM))
    return -EINVAL;
  // we are purposefully ignoring the protocol argument
  if (UNLIKELY(protocol < 0))
    return -EPROTONOSUPPORT;

  return [](const int type)->int{
    switch(type)
    {
      case SOCK_STREAM:
        return FD_map::_open<TCP_FD>().get_id();
      case SOCK_DGRAM:
        return FD_map::_open<UDP_FD>().get_id();
      default:
        return -EINVAL;
    }
  }(type);
}

static long sock_connect(int sockfd, const struct sockaddr *addr,
                         socklen_t addrlen)
{
  if(auto* fildes = FD_map::_get(sockfd); fildes)
    return fildes->connect(addr, addrlen);

  return -EBADF;
}

static long sock_bind(int sockfd, const struct sockaddr *addr,
                      socklen_t addrlen)
{
  if(auto* fildes = FD_map::_get(sockfd); fildes)
    return fildes->bind(addr, addrlen);

  return -EBADF;
}

static long sock_sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
  if(auto* fildes = FD_map::_get(sockfd); fildes)
    return fildes->sendmsg(msg, flags);

  return -EBADF;
}

static ssize_t sock_sendto(int sockfd, const void *buf, size_t len, int flags,
                           const struct sockaddr *dest_addr, socklen_t addrlen)
{
  if(auto* fildes = FD_map::_get(sockfd); fildes)
    return fildes->sendto(buf, len, flags, dest_addr, addrlen);

  return -EBADF;
}

static ssize_t sock_recvfrom(int sockfd, void *buf, size_t len, int flags,
                             struct sockaddr *src_addr, socklen_t *addrlen)
{
  if(auto* fildes = FD_map::_get(sockfd); fildes)
    return fildes->recvfrom(buf, len, flags, src_addr, addrlen);

  return -EBADF;
}

static long sock_listen(int sockfd, int backlog)
{
  if(auto* fildes = FD_map::_get(sockfd); fildes)
    return fildes->listen(backlog);

  return -EBADF;
}

static long sock_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  if(auto* fildes = FD_map::_get(sockfd); fildes)
    return fildes->accept(addr, addrlen);

  return -EBADF;
}

static long sock_shutdown(int sockfd, int how)
{
  if(auto* fildes = FD_map::_get(sockfd); fildes)
    return fildes->shutdown(how);

  return -EBADF;
}

extern "C" {
long socketcall_socket(int domain, int type, int protocol)
{
  return strace(sock_socket, "socket", domain, type, protocol);
}

long socketcall_getsockopt(int sockfd,
    int level, int optname, void *optval, socklen_t *optlen)
{
  return -ENOSYS;
}
long socketcall_setsockopt(int sockfd,
    int level, int optname, const void *optval, socklen_t optlen)
{
  return -ENOSYS;
}
long socketcall_getsockname(int sockfd,
    struct sockaddr *addr, socklen_t *addrlen)

{
  return -ENOSYS;
}
long socketcall_getpeername(int sockfd,
    struct sockaddr *addr, socklen_t *addrlen)

{
  return -ENOSYS;
}

long socketcall_connect(int sockfd, const struct sockaddr *addr,
                        socklen_t addrlen)
{
  return strace(sock_connect, "connect", sockfd, addr, addrlen);
}

long socketcall_bind(int sockfd, const struct sockaddr *addr,
                     socklen_t addrlen)
{
  return strace(sock_bind, "bind", sockfd, addr, addrlen);
}

long socketcall_sendmsg(int sockfd, const struct msghdr *msg, int flags)
{
  return strace(sock_sendmsg, "sendmsg", sockfd, msg, flags);
}

ssize_t socketcall_sendto(int sockfd, const void *buf, size_t len, int flags,
                          const struct sockaddr *dest_addr, socklen_t addrlen)
{
  return strace(sock_sendto, "sendto", sockfd, buf, len, flags, dest_addr, addrlen);
}

ssize_t socketcall_recvfrom(int sockfd, void *buf, size_t len, int flags,
                            struct sockaddr *src_addr, socklen_t *addrlen)
{
  return strace(sock_recvfrom, "recvfrom", sockfd, buf, len, flags, src_addr, addrlen);
}

long socketcall_listen(int sockfd, int backlog)
{
  return strace(sock_listen, "listen", sockfd, backlog);
}

long socketcall_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
  return strace(sock_accept, "accept", sockfd, addr, addrlen);
}

long socketcall_shutdown(int sockfd, int how)
{
  return strace(sock_shutdown, "shutdown", sockfd, how);
}
} // < extern "C"
