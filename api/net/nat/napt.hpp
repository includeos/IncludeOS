// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_NAT_NAPT_HPP
#define NET_NAT_NAPT_HPP

#include <map>
#include <net/port_util.hpp>
#include <net/inet>
#include <net/tcp/tcp.hpp>

namespace net {
namespace nat {

/**
 * @brief      Network Address Port Translator
 */
class NAPT {
public:
  using Stack = Inet<IP4>;
  using Translation_table = std::map<uint16_t, Socket>;

public:

  // NAT
  IP4::IP_packet_ptr nat(IP4::IP_packet_ptr pkt, const Stack& inet);

  // Replace source socket with external (this) address and random port
  IP4::IP_packet_ptr snat(IP4::IP_packet_ptr pkt, const Stack& inet);

  // Replace source address with external address from table
  IP4::IP_packet_ptr dnat(IP4::IP_packet_ptr pkt, const Stack& inet);

  void add_entry(uint16_t port, Socket sock)
  {
    // Bind the port
    tcp_ports.bind(port);
    // Add the entry
    tcp_trans.emplace(port, sock);

    //printf("NAT entry: %s => %u\n", sock.to_string().c_str(), port);
  }

private:
  Port_util tcp_ports;
  Port_util udp_ports;

  Translation_table tcp_trans;
  Translation_table udp_trans;

  // Source NAT
  void snat(tcp::Packet& pkt, ip4::Addr src_ip);

  // Destination NAT
  void dnat(tcp::Packet& pkt);

  void recalculate_checksum(tcp::Packet& pkt) noexcept
  {
    pkt.set_checksum(0);
    pkt.set_ip_checksum();
    pkt.set_checksum(TCP::checksum(pkt));
  }

}; // < class NAPT

} // < namespace nat
} // < namespace net

#endif
