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


#ifndef NET_INET_HPP
#define NET_INET_HPP

#include <net/inet_common.hpp>

namespace net {
  
  class TCP;
  class UDP;
  class DHClient;
  
  /** An abstract IP-stack interface  */  
  template <typename LINKLAYER, typename IPV >
  class Inet {
  public:
    using Stack = Inet<LINKLAYER, IPV>;
    template <typename IPv>
    using resolve_func = delegate<void(Stack&, const std::string&, typename IPv::addr)>;
    
    virtual const typename IPV::addr& ip_addr() = 0;
    virtual const typename IPV::addr& netmask() = 0;
    virtual const typename IPV::addr& router() = 0;
    virtual const typename LINKLAYER::addr& link_addr() = 0;
    virtual LINKLAYER& link() = 0;
    virtual IPV& ip_obj() = 0;
    virtual TCP& tcp() = 0;
    virtual UDP& udp() = 0;
    virtual std::shared_ptr<DHClient> dhclient() = 0;
    
    virtual uint16_t MTU() const = 0;
    
    virtual Packet_ptr createPacket(size_t size) = 0;
    
    virtual void
    resolve(const std::string& hostname,
            resolve_func<IPV>  func) = 0;
    
    virtual void
    set_dns_server(typename IPV::addr server) = 0;
    
    virtual void network_config(
        typename IPV::addr ip, 
        typename IPV::addr nmask, 
        typename IPV::addr router,
        typename IPV::addr dnssrv) = 0;
  };

}

#endif
