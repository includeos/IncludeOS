// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
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
#ifndef NET_IP4_UDP_SOCKET_HPP
#define NET_IP4_UDP_SOCKET_HPP

#include "common.hpp"
#include "header.hpp"
#include "packet_view.hpp"

#include <net/socket.hpp>

#include <string>

namespace net {
  class UDP;
}
namespace net::udp
{
  class Socket
  {
  public:
    using multicast_group_addr = ip4::Addr;

    using recvfrom_handler  = delegate<void(addr_t, port_t, const char*, size_t)>;

    // constructors
    Socket(UDP&, net::Socket socket);
    Socket(const Socket&) = delete;
    // ^ DON'T USE THESE. We could create our own allocators just to prevent
    // you from creating sockets, but then everyone is wasting time.
    // These are public to allow us to use emplace(...).
    // Use Stack.udp().bind(port) to get a valid Socket<UDP> reference.

    // functions
    void on_read(recvfrom_handler callback)
    { on_read_handler = callback; }

    void sendto(addr_t destIP, port_t port,
                const void* buffer, size_t length,
                sendto_handler cb = nullptr,
                error_handler ecb = nullptr);

    void bcast(addr_t srcIP, port_t port,
               const void* buffer, size_t length,
               sendto_handler cb = nullptr,
               error_handler ecb = nullptr);

    void close();

    void join(multicast_group_addr);
    void leave(multicast_group_addr);

    // stuff
    addr_t local_addr() const
    { return socket_.address(); }

    port_t local_port() const
    { return socket_.port(); }

    const net::Socket& local() const
    { return socket_; }

  private:
    void internal_read(const Packet_view&);

    UDP&    udp_;
    net::Socket  socket_;
    recvfrom_handler on_read_handler =
      [] (addr_t, port_t, const char*, size_t) {};

    const bool is_ipv6_;
    bool reuse_addr;
    bool loopback; // true means multicast data is looped back to sender

    friend class net::UDP;
    friend class std::allocator<net::udp::Socket>;
  };
}

#endif
