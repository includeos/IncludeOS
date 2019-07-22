
#pragma once
#ifndef POSIX_SYSLOG_UDP_SOCKET_HPP
#define POSIX_SYSLOG_UDP_SOCKET_HPP

#include <posix/unix_fd_impl.hpp>
#include <net/inet>
#include <net/udp/udp.hpp>

class Syslog_UDP_socket : public Unix_FD_impl {
public:
  inline Syslog_UDP_socket(net::Inet& net,
                           const net::ip4::Addr raddr, const uint16_t rport);

  inline long    connect(const struct sockaddr *, socklen_t) override;
  inline ssize_t sendto(const void* buf, size_t, int fl,
                        const struct sockaddr* addr, socklen_t addrlen) override;
  inline int     close() override;

  inline ~Syslog_UDP_socket();

private:
  net::Inet&  stack;
  net::udp::Socket*     udp;
  const net::ip4::Addr  addr;
  const uint16_t        port;
};

Syslog_UDP_socket::Syslog_UDP_socket(net::Inet& net,
                                     const net::ip4::Addr raddr,
                                     const uint16_t rport)
  : stack{net}, udp{nullptr}, addr{raddr}, port{rport}
{
}

long Syslog_UDP_socket::connect(const struct sockaddr*, socklen_t)
{
  // so we can use the call to connect to bind to a UDP socket.
  // we can also hopefully assume that the network is configured
  // and bind wont cause any issues.
  Expects(stack.is_configured());
  if(udp == nullptr)
    udp = &stack.udp().bind();
  return 0;
}

ssize_t Syslog_UDP_socket::sendto(const void* buf, size_t len, int /*fl*/,
                                  const struct sockaddr*, socklen_t)
{
  Expects(udp != nullptr);
  udp->sendto(addr, port, buf, len);
  return len;
}

int Syslog_UDP_socket::close()
{
  if(udp)
  {
    udp->close();
    udp = nullptr;
  }
  return 0;
}

Syslog_UDP_socket::~Syslog_UDP_socket()
{
  if(udp != nullptr)
    udp->close();
}

#endif
