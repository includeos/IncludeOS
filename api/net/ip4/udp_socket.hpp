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
#include "udp.hpp"
#include <string>

namespace net
{
  class UDPSocket
  {
  public:
    typedef UDP::port_t port_t;
    typedef IP4::addr addr_t;
    typedef IP4::addr multicast_group_addr;

    typedef delegate<void(addr_t, port_t, const char*, size_t)> recvfrom_handler;
    typedef UDP::sendto_handler sendto_handler;

    // constructors
    UDPSocket(UDP&, port_t port);
    UDPSocket(const UDPSocket&) = delete;
    // ^ DON'T USE THESE. We could create our own allocators just to prevent
    // you from creating sockets, but then everyone is wasting time.
    // These are public to allow us to use emplace(...).
    // Use Stack.udp().bind(port) to get a valid Socket<UDP> reference.

    // functions
    void on_read(recvfrom_handler callback)
    {
      on_read_handler = callback;
    }
    void sendto(addr_t destIP, port_t port,
                const void* buffer, size_t length,
                sendto_handler cb = [] {});
    void bcast(addr_t srcIP, port_t port,
               const void* buffer, size_t length,
               sendto_handler cb = [] {});
    void close();

    void join(multicast_group_addr);
    void leave(multicast_group_addr);

    // stuff
    addr_t local_addr() const
    {
      return udp_.local_ip();
    }
    port_t local_port() const
    {
      return l_port;
    }

    UDP& udp(){
      return udp_;
    }

  private:
    void packet_init(UDP::Packet_ptr, addr_t, addr_t, port_t, uint16_t);
    void internal_read(UDP::Packet_ptr);

    UDP& udp_;
    port_t l_port;
    recvfrom_handler on_read_handler =
      [] (addr_t, port_t, const char*, size_t) {};

    bool reuse_addr;
    bool loopback; // true means multicast data is looped back to sender

    friend class UDP;
    friend class std::allocator<UDPSocket>;
  };
}

#endif
