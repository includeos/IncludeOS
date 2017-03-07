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

// #define DEBUG
#include <net/ip4/icmpv4.hpp>

namespace net {

  ICMPv4::ICMPv4(Stack& inet) :
    inet_{inet}
{}

  void ICMPv4::receive(Packet_ptr pckt) {

    if ((size_t)pckt->size() < sizeof(IP4::header) + icmp4::Packet::header_size()) // Drop if not a full header
      return;

    auto pckt_ip4 = static_unique_ptr_cast<PacketIP4>(std::move(pckt));
    auto req = icmp4::Packet(std::move(pckt_ip4));

    switch(req.type()) {

    case (icmp4::Type::ECHO):
      debug("<ICMP> PING from %s\n", req.ip().src().str().c_str());
      ping_reply(req);
      break;

    case (icmp4::Type::ECHO_REPLY):
      debug("<ICMP> PING Reply from %s\n", req.ip().src().str().c_str());
      break;
    }
  }

  void ICMPv4::ping_reply(icmp4::Packet& req) {

    // Provision new IP4-packet
    icmp4::Packet res(inet_.ip_packet_factory());

    // Populate response IP header
    res.ip().set_src(inet_.ip_addr());
    res.ip().set_dst(req.ip().src());

    // Populate response ICMP header
    res.set_type(icmp4::Type::ECHO_REPLY);
    res.set_code(0);
    res.set_id(req.id());
    res.set_sequence(req.sequence());

    debug("<ICMP> Transmitting answer to %s\n", res.ip().dst().str().c_str());

    // Copy payload from old to new packet
    res.set_payload(req.payload());

    // Add checksum
    res.set_checksum();

    debug("<ICMP> Response size: %i payload size: %i, checksum: 0x%x\n",
          res.ip().size(), res.payload().size(), res.compute_checksum());

    network_layer_out_(res.release());
  }


} //< namespace net
