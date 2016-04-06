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
    typedef UDP::Packet_ptr PacketUDP_ptr;
    
    using Stack = Inet<LinkLayer, IP4>;
    
    typedef delegate<int(addr_t, port_t, const char*, int)> recvfrom_handler;
    typedef delegate<int(addr_t, port_t, const char*, int)> sendto_handler;
    
    // constructors
    UDPSocket(Stack&, port_t port);
    UDPSocket(const UDPSocket&) = delete;
    // ^ DON'T USE THESE. We could create our own allocators just to prevent
    // you from creating sockets, but then everyone is wasting time.
    // These are public to allow us to use emplace(...).
    // Use Stack.udp().bind(port) to get a valid Socket<UDP> reference.
    
    // functions
    void onRead(recvfrom_handler func)
    {
      on_read = func;
    }
    void onWrite(sendto_handler func)
    {
      on_send = func;
    }
    int sendto(addr_t destIP, port_t port, 
               const void* buffer, int length);
    int bcast(addr_t srcIP, port_t port, 
              const void* buffer, int length);
    void close();
    
    void join(multicast_group_addr);
    void leave(multicast_group_addr);
    
    // stuff
    addr_t local_addr() const
    {
      return stack.ip_addr();
    }
    port_t local_port() const
    {
      return l_port;
    }
    
  private:
    void packet_init(PacketUDP_ptr, addr_t, addr_t, port_t, uint16_t);
    int  internal_read(PacketUDP_ptr);
    int  internal_write(addr_t, addr_t, port_t, const uint8_t*, int);
    
    Stack& stack;
    port_t l_port;
    recvfrom_handler on_read = [](addr_t, port_t, const char*, int)->int{ return 0; };
    sendto_handler   on_send = [](addr_t, port_t, const char*, int)->int{ return 0; };
    
    bool reuse_addr;
    bool loopback; // true means multicast data is looped back to sender
    
    friend class UDP;
    friend class std::allocator<UDPSocket>;
  };
}

#endif
