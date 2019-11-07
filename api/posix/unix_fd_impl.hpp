
#pragma once
#ifndef POSIX_UNIX_FD_IMPL_HPP
#define POSIX_UNIX_FD_IMPL_HPP

#include <sys/socket.h>
class Unix_FD_impl {
public:
  virtual long    connect(const struct sockaddr *, socklen_t) = 0;
  virtual ssize_t sendto(const void* buf, size_t, int fl,
                         const struct sockaddr* addr, socklen_t addrlen) = 0;
  virtual int     close() = 0;
  virtual ~Unix_FD_impl() = default;
};

#endif
