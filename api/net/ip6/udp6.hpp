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

#ifndef NET_IP6_UDP_HPP
#define NET_IP6_UDP_HPP

#include <deque>
#include <map>
#include <cstring>
#include <unordered_map>

#include "../inet.hpp"
#include "ip6.hpp"
#include <net/packet.hpp>
#include <net/socket.hpp>
#include <util/timer.hpp>
#include <rtc>

namespace net
{
  class PacketUDP6;
  class UDPSocket;

  class UDPv6
  {
  public:
    using addr_t = IP6::addr;
    using port_t = uint16_t;

    using Packet_ptr    = std::unique_ptr<PacketUDP, std::default_delete<net::Packet>>;
    using Stack         = IP6::Stack;

    using Sockets       = std::map<Socket, UDPSocket>;

    typedef delegate<void()> sendto_handler;
    typedef delegate<void(const Error&)> error_handler;
  };
}
#endif
