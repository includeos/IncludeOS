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

//#define IP6_DEBUG 1
#ifdef IP6_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

#include <net/ip6/ip6.hpp>
#include <net/inet>
#include <net/ip6/packet_ip6.hpp>
#include <net/ip6/header.hpp>
#include <net/packet.hpp>
#include <statman>

namespace net
{
  const ip6::Addr IP6::ADDR_ANY(0, 0, 0, 0);
  const ip6::Addr IP6::ADDR_LOOPBACK(0, 0, 0, 1);

  IP6::IP6(Stack& inet) noexcept :
  stack_            {inet},
  packets_rx_       {Statman::get().create(Stat::UINT64, inet.ifname() + ".ip6.packets_rx").get_uint64()},
  packets_tx_       {Statman::get().create(Stat::UINT64, inet.ifname() + ".ip6.packets_tx").get_uint64()},
  packets_dropped_  {Statman::get().create(Stat::UINT32, inet.ifname() + ".ip6.packets_dropped").get_uint32()}
  {}

  IP6::IP_packet_ptr IP6::drop(IP_packet_ptr ptr, Direction direction, Drop_reason reason) {
    packets_dropped_++;

    if (drop_handler_)
        drop_handler_(std::move(ptr), direction, reason);

    return nullptr;
  }

  IP6::IP_packet_ptr IP6::drop_invalid_in(IP6::IP_packet_ptr packet)
  {
    IP6::Direction up = IP6::Direction::Upstream;

    if (UNLIKELY(not packet->is_ipv6()))
        return drop(std::move(packet), up, Drop_reason::Wrong_version);

    /* TODO: Add more checks */
    return packet;
  }

  IP6::IP_packet_ptr IP6::drop_invalid_out(IP6::IP_packet_ptr packet)
  {
    if (packet->ip_dst().is_loopback()) {
      drop(std::move(packet), Direction::Downstream, Drop_reason::Bad_destination);
    }
    return packet;
  }

  /* TODO: Check RFC */
  bool IP6::is_for_me(ip6::Addr dst) const
  {
    return stack_.is_valid_source6(dst)
      or local_ip() == ADDR_ANY;
  }

  void PacketIP6::calculate_payload_offset()
  {
    const ptrdiff_t size = this->data_end() - this->ext_hdr_start();
    const auto* reader = this->ext_hdr_start();
    auto next_proto = this->next_protocol();
    ssize_t pl_off = sizeof(ip6::Header);

    while (next_proto != Protocol::IPv6_NONXT)
    {
        if (next_proto != Protocol::HOPOPT &&
            next_proto != Protocol::OPTSV6)
        {
            PRINT("Done parsing extension header, next proto: %d\n", next_proto);
            this->set_payload_offset(pl_off);
            return;
        }
        // bounds check
        if (reader + sizeof(ip6::Extension_header) >= this->data_end())
        {
          break;
        }
        auto& ext = *(ip6::Extension_header*)reader;
        next_proto = ext.proto();
        pl_off += ext.size();
        reader += ext.size();
    }
    // bounds-check final payload offset
    if (pl_off > size) pl_off = size;
    this->set_payload_offset(pl_off);
  }

  void IP6::receive(Packet_ptr pckt, const bool link_bcast)
  {
    auto packet = static_unique_ptr_cast<net::PacketIP6>(std::move(pckt));
    // this will calculate exthdr length and set payload correctly
    packet->calculate_payload_offset();

    PRINT("<IP6 Receive> Source IP: %s, Dest.IP: %s, Next header: %d,"
            "Payload len: %u, Hop limit: %d, version: %d, tc: %u, fl: %u\n",
           packet->ip_src().str().c_str(),
           packet->ip_dst().str().c_str(),
           (int) packet->next_header(),
           packet->payload_length(), packet->hop_limit(),
           packet->ip6_version(), packet->traffic_class(), packet->flow_label());

    switch (packet->next_protocol()) {
    case Protocol::HOPOPT:
       PRINT("Type: ICMP6 hop by hop option\n"); break;
    case Protocol::OPTSV6:
       PRINT("Type: ICMP6 option: %d\n", (int)packet->next_protocol()); break;
    case Protocol::IPv6_NONXT:
       PRINT("Type: ICMP6 No Next\n"); break;
    case Protocol::ICMPv6:
       PRINT("Type: ICMP6\n"); break;
    case Protocol::IPv4:
       PRINT("Type: IPv4\n"); break;
    case Protocol::UDP:
       PRINT("Type: UDP\n"); break;
    case Protocol::TCP:
       PRINT("Type: TCP\n"); break;
    default:
       PRINT("Type: UNKNOWN %hhu. Dropping. \n", packet->next_protocol());
    }

    // Stat increment packets received
    packets_rx_++;

    packet = drop_invalid_in(std::move(packet));
    if (UNLIKELY(packet == nullptr)) return;

    /* PREROUTING */
    // Track incoming packet if conntrack is active
    Conntrack::Entry_ptr ct = (stack_.conntrack())
      ? stack_.conntrack()->in(*packet) : nullptr;
    auto res = prerouting_chain_(std::move(packet), stack_, ct);
    if (UNLIKELY(res == Filter_verdict_type::DROP)) return;

    Ensures(res.packet != nullptr);
    packet = res.release();

    // Drop / forward if my ip address doesn't match dest.
    if(not is_for_me(packet->ip_dst()))
    {
      // Forwarding disabled
      if (not forward_packet_)
      {
        PRINT("Dropping packet \n");
        drop(std::move(packet), Direction::Upstream, Drop_reason::Bad_destination);
      }
      // Forwarding enabled
      else
      {
        PRINT("Forwarding packet \n");
        forward_packet_(std::move(packet), stack_, ct);
      }
      return;
    }

    PRINT("* Packet was for me\n");

    /* INPUT */
    // Confirm incoming packet if conntrack is active
    auto& conntrack = stack_.conntrack();
    if(conntrack) {
      ct = (ct != nullptr) ?
        conntrack->confirm(ct->second, ct->proto) : conntrack->confirm(*packet);
    }
    if(stack_.conntrack())
      stack_.conntrack()->confirm(*packet); // No need to set ct again
    res = input_chain_(std::move(packet), stack_, ct);
    if (UNLIKELY(res == Filter_verdict_type::DROP)) return;

    Ensures(res.packet != nullptr);
    packet = res.release();

    auto next_proto = packet->ip_protocol();

    switch (next_proto) {
    case Protocol::IPv6_NONXT:
      /* Nothing after the icmp header */
      break;
    case Protocol::ICMPv6:
      PRINT("ICMPv6: %d\n", packet->size());
      icmp_handler_(std::move(packet));
      break;
    case Protocol::UDP:
      udp_handler_(std::move(packet));
      break;
    case Protocol::TCP:
      tcp_handler_(std::move(packet));
      break;
    default:
      // Send ICMP error of type Destination Unreachable and code PROTOCOL
      // @note: If dest. is broadcast or multicast it should be dropped by now
      //stack_.icmp().destination_unreachable(std::move(packet), icmp6::code::Dest_unreachable::PROTOCOL);

      PRINT("Unknown next proto. Dropping packet\n");
      drop(std::move(packet), Direction::Upstream, Drop_reason::Unknown_proto);
      break;
    }
  }

  void IP6::transmit(Packet_ptr pckt) {
    assert((size_t)pckt->size() > sizeof(header));

    auto packet = static_unique_ptr_cast<PacketIP6>(std::move(pckt));

    /*
     * RFC 1122 p. 30
     * When a host sends any datagram, the IP source address MUST
       be one of its own IP addresses (but not a broadcast or
       multicast address).
    */
    if (UNLIKELY(not stack_.is_valid_source6(packet->ip_src()))) {
      PRINT("<IP6> Drop bad source egress: src=%s list:\n%s\n",
        packet->ip_src().to_string().c_str(), addr_list_.to_string().c_str());
      drop(std::move(packet), Direction::Downstream, Drop_reason::Bad_source);
      return;
    }

    packet->make_flight_ready();

    /* OUTPUT */
    Conntrack::Entry_ptr ct =
      (stack_.conntrack()) ? stack_.conntrack()->in(*packet) : nullptr;
    auto res = output_chain_(std::move(packet), stack_, ct);
    if (UNLIKELY(res == Filter_verdict_type::DROP)) return;

    Ensures(res.packet != nullptr);
    packet = res.release();

    if (forward_packet_) {
      forward_packet_(std::move(packet), stack_, ct);
      return;
    }
    ship(std::move(packet), IP6::ADDR_ANY, ct);
  }

  void IP6::ship(Packet_ptr pckt, ip6::Addr next_hop, Conntrack::Entry_ptr ct)
  {
    auto packet = static_unique_ptr_cast<PacketIP6>(std::move(pckt));

    // Send loopback packets right back
    if (UNLIKELY(stack_.is_valid_source(packet->ip_dst()))) {
      PRINT("<IP6> Destination address is loopback \n");
      IP6::receive(std::move(packet), false);
      return;
    }

    // Filter illegal egress packets
    packet = drop_invalid_out(std::move(packet));
    if (packet == nullptr) return;

    if (next_hop == ip6::Addr::addr_any)
    {
      auto dst = packet->ip_dst();
      next_hop = dst.is_linklocal() ? dst : stack_.ndp().next_hop(dst);
      PRINT("<IP6> Nexthop for %s: %s\n", dst.to_string().c_str(), next_hop.to_string().c_str());

      if(UNLIKELY(next_hop == ip6::Addr::addr_any)) {
        PRINT("<IP6> Next_hop calculated to 0, dropping\n");
        drop(std::move(packet), Direction::Downstream, Drop_reason::Bad_destination);
        return;
      }
    }

    // Stat increment packets transmitted
    packets_tx_++;

    ndp_out_(std::move(packet), next_hop, MAC::EMPTY);
  }

  const ip6::Addr IP6::local_ip() const {
    return stack_.ip6_addr();
  }

  uint16_t IP6::MDDS() const
  { return stack_.MTU() - sizeof(ip6::Header); }
}
