
#pragma once
#ifndef POSIX_SYSLOG_PRINT_SOCKET_HPP
#define POSIX_SYSLOG_PRINT_SOCKET_HPP

#include <posix/unix_fd_impl.hpp>

class Syslog_print_socket : public Unix_FD_impl {
public:
  Syslog_print_socket() = default;
  inline long    connect(const struct sockaddr *, socklen_t) override;
  inline ssize_t sendto(const void* buf, size_t, int fl,
                 const struct sockaddr* addr, socklen_t addrlen) override;
  inline int     close() override { return 0; }
  ~Syslog_print_socket() = default;
};

long Syslog_print_socket::connect(const struct sockaddr *, socklen_t)
{
  // connect doesnt really mean anything here
  return 0;
}

#include <os.hpp>
ssize_t Syslog_print_socket::sendto(const void* buf, size_t len, int /*fl*/,
                                    const struct sockaddr* /*addr*/,
                                    socklen_t /*addrlen*/)
{
  os::print(reinterpret_cast<const char*>(buf), len);
  return len;
}

#endif
