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
#ifndef NET_DHCP_DH4CLIENT_HPP
#define NET_DHCP_DH4CLIENT_HPP

#include "../packet.hpp"
#include <info>

namespace net
{
  class UDPSocket;
  
  template <typename LINK, typename IPV>
  class Inet;
  
  class DHClient
  {
  public:
    using Stack = Inet<LinkLayer, IP4>;
    using On_config = delegate<void(Stack&)>;
    
    DHClient() = delete;
    DHClient(DHClient&) = delete;
    DHClient(Stack& inet)
      : stack(inet)  {}
    
    Stack& stack;
    void negotiate(); // --> offer
    inline void on_config(On_config handler){
      config_handler = handler;
    }
    
  private:
    void offer(UDPSocket&, const char* data, size_t len);
    void request(UDPSocket&);   // --> acknowledge
    void acknowledge(const char* data, size_t len);
    
    uint32_t  xid;
    IP4::addr ipaddr, netmask, router, dns_server;
    uint32_t  lease_time;
    On_config config_handler = [](Stack&){ INFO("DHCPv4::On_config","Config complete"); };
  };
}

#endif
