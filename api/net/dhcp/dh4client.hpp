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


#ifndef NET_DHCP_DH4CLIENT_HPP
#define NET_DHCP_DH4CLIENT_HPP

#define DEBUG
#include "../packet.hpp"

#include <debug>
#include <info>

namespace net
{
  template <typename T>
  class Socket;
  class UDP;
  
  template <typename LINK, typename IPV>
  class Inet;
  
  class DHClient
  {
  public:
    using Stack = Inet<LinkLayer, IP4>;
    using On_config = delegate<void(Stack&)>;
    
    DHClient() = delete;
    DHClient(DHClient&) = delete;
    DHClient(Stack&);
    
    Stack& stack;
    void negotiate(); // --> offer
    inline void on_config(On_config handler){
      config_handler = handler;
    }
    
  private:
    void offer(Socket<UDP>&, const char* data, int len);
    void request(Socket<UDP>&);   // --> acknowledge
    void acknowledge(const char* data, int len);
    
    uint32_t  xid;
    IP4::addr ipaddr, netmask, router, dns_server;
    uint32_t  lease_time;
    On_config config_handler = [](Stack&){ INFO("DHCPv4::On_config","Config complete"); };
  };
	
  inline DHClient::DHClient(Stack& inet)
    : stack(inet)  {}
}

#endif
