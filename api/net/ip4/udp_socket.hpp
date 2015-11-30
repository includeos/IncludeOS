// Part of the IncludeOS Unikernel  - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud. 
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


#ifndef NET_IP4_UDP_SOCKET_HPP
#define NET_IP4_UDP_SOCKET_HPP
#include "udp.hpp"
#include <string>

namespace net
{
  template <class T>
  class Socket;
  
  template<>
  class Socket<UDP>
  {
  public:
    typedef UDP::port port;
    typedef IP4::addr addr;
    typedef IP4::addr multicast_group_addr;
    
    typedef delegate<int(Socket<UDP>&, addr, port, const char*, int)> recvfrom_handler;
    typedef delegate<int(Socket<UDP>&, addr, port, const char*, int)> sendto_handler;
    
    // constructors
    Socket<UDP>(Inet<LinkLayer,IP4>&, port port);
    Socket<UDP>(const Socket<UDP>&) = delete;
    // ^ DON'T USE THESE. We could create our own allocators just to prevent
    // you from creating sockets, but then everyone is wasting time.
    // These are public to allow us to use emplace(...).
    // Use Stack.udp().bind(port) to get a valid Socket<UDP> reference.
    
    // functions
    inline void onRead(recvfrom_handler func)
    {
      on_read = func;
    }
    inline void onWrite(sendto_handler func)
    {
      on_send = func;
    }
    int sendto(addr destIP, port port, 
               const void* buffer, int length);
    int bcast(addr srcIP, port port, 
              const void* buffer, int length);
    void close();
    
    void join(multicast_group_addr&);
    void leave(multicast_group_addr&);
    
    // stuff
    addr local_addr() const
    {
      return stack.ip_addr();
    }
    port local_port() const
    {
      return l_port;
    }
    
  private:
    void packet_init(std::shared_ptr<PacketUDP>, addr, addr, port, uint16_t);
    int  internal_read(std::shared_ptr<PacketUDP>);
    int  internal_write(addr, addr, port, const uint8_t*, int);
    
    Inet<LinkLayer,IP4>& stack;
    port l_port;
    recvfrom_handler on_read = [](Socket<UDP>&, addr, port, const char*, int)->int{ return 0; };
    sendto_handler   on_send = [](Socket<UDP>&, addr, port, const char*, int)->int{ return 0; };
    
    bool reuse_addr;
    bool loopback; // true means multicast data is looped back to sender
    
    friend class UDP;
    friend class std::allocator<Socket>;
  };
}

#endif
