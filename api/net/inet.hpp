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
    using resolve_func = delegate<void(typename IPv::addr)>;

    virtual typename IPV::addr ip_addr() = 0;
    virtual typename IPV::addr netmask() = 0;
    virtual typename IPV::addr router()  = 0;
    virtual std::string ifname() const = 0;
    virtual typename LINKLAYER::addr link_addr() = 0;

    virtual LINKLAYER& link()   = 0;
    virtual IPV&       ip_obj() = 0;
    virtual TCP&       tcp()    = 0;
    virtual UDP&       udp()    = 0;

    virtual constexpr uint16_t MTU() const = 0;

    virtual Packet_ptr create_packet(size_t size) = 0;

    virtual void resolve(const std::string& hostname, resolve_func<IPV> func) = 0;

    virtual void set_router(typename IPV::addr server) = 0;

    virtual void set_dns_server(typename IPV::addr server) = 0;

    virtual void network_config(typename IPV::addr ip,
                                typename IPV::addr nmask,
                                typename IPV::addr router,
                                typename IPV::addr dnssrv) = 0;

    /** Event triggered when there are available buffers in the transmit queue */
    virtual void on_transmit_queue_available(transmit_avail_delg del) = 0;

    /** Number of packets the transmit queue has room for */
    virtual size_t transmit_queue_available() = 0;

    /** Number of buffers available in the bufstore */
    virtual size_t buffers_available() = 0;

  }; //< class Inet<LINKLAYER, IPV>
} //< namespace net

#endif
