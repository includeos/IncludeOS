// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
