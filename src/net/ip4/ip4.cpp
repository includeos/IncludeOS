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

//#define IP_DEBUG 1
#ifdef IP_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

#include <net/ip4/ip4.hpp>
#include <net/inet>
#include <net/ip4/packet_ip4.hpp>
#include <net/packet.hpp>
#include <statman>
#include <net/ip4/icmp4.hpp>

namespace net {

  const ip4::Addr ip4::Addr::addr_any{0};
  const IP4::addr IP4::ADDR_ANY(0);
  const IP4::addr IP4::ADDR_BCAST(0xff,0xff,0xff,0xff);

  IP4::IP4(Stack& inet) noexcept :
  packets_rx_       {Statman::get().create(Stat::UINT64, inet.ifname() + ".ip4.packets_rx").get_uint64()},
  packets_tx_       {Statman::get().create(Stat::UINT64, inet.ifname() + ".ip4.packets_tx").get_uint64()},
  packets_dropped_  {Statman::get().create(Stat::UINT32, inet.ifname() + ".ip4.packets_dropped").get_uint32()},
  stack_            {inet}
  {}


  IP4::IP_packet_ptr IP4::drop(IP_packet_ptr ptr, Direction direction, Drop_reason reason) {
    packets_dropped_++;

    if(drop_handler_)
      drop_handler_(std::move(ptr), direction, reason);

    return nullptr;
  }


  IP4::IP_packet_ptr IP4::drop_invalid_in(IP4::IP_packet_ptr packet)
  {
    IP4::Direction up = IP4::Direction::Upstream;

    // RFC-1122 3.2.1.1, Silently discard Version != 4
    if (UNLIKELY(not packet->is_ipv4()))
      return drop(std::move(packet), up, Drop_reason::Wrong_version);

    // RFC-1122 3.2.1.2, Verify IP checksum, silently discard bad dgram
    if (UNLIKELY(packet->compute_ip_checksum() != 0))
      return drop(std::move(packet), up, Drop_reason::Wrong_checksum);

    // RFC-1122 3.2.1.3, Silently discard datagram with bad src addr
    // Here dropping if the source ip address is a multicast address or is this interface's broadcast address
    if (UNLIKELY(packet->ip_src().is_multicast() or packet->ip_src() == IP4::ADDR_BCAST or
      packet->ip_src() == stack_.broadcast_addr())) {
      return drop(std::move(packet), up, Drop_reason::Bad_source);
    }

    return packet;
  }


  IP4::IP_packet_ptr IP4::drop_invalid_out(IP4::IP_packet_ptr packet)
  {
    // RFC-1122 3.2.1.7, MUST NOT send packet with TTL of 0
    if (packet->ip_ttl() == 0)
      return drop(std::move(packet), Direction::Downstream, Drop_reason::TTL0);

    // RFC-1122 3.2.1.7, MUST NOT send packet addressed to 127.*
    if (packet->ip_dst().part(0) == 127)
      drop(std::move(packet), Direction::Downstream, Drop_reason::Bad_destination);

    return packet;
  }

  /*
   * RFC 1122, p. 30
   * An incoming datagram is destined
     for the host if the datagram's destination address field is:
     (1)  (one of) the host's IP address(es); or
     (2)  an IP broadcast address valid for the connected
          network; or
     (3)  the address for a multicast group of which the host is
          a member on the incoming physical interface.
  */
  bool IP4::is_for_me(ip4::Addr dst) const
  {
    return stack_.is_valid_source(dst)
      or dst == stack_.broadcast_addr()
      or dst == ADDR_BCAST;
  }

  void IP4::receive(Packet_ptr pckt, [[maybe_unused]]const bool link_bcast)
  {
    // Cast to IP4 Packet
    auto packet = static_unique_ptr_cast<net::PacketIP4>(std::move(pckt));

    PRINT("<IP4 Receive> Source IP: %s Dest.IP: %s Type: 0x%x LinkBcast: %d ",
           packet->ip_src().str().c_str(),
           packet->ip_dst().str().c_str(),
           (int) packet->ip_protocol(),
           link_bcast);
    switch (packet->ip_protocol()) {
    case Protocol::ICMPv4:
       PRINT("Type: ICMP\n"); break;
    case Protocol::UDP:
       PRINT("Type: UDP\n"); break;
    case Protocol::TCP:
       PRINT("Type: TCP\n"); break;
    default:
       PRINT("Type: UNKNOWN %hhu. Dropping. \n", packet->ip_protocol());
    }

    // Stat increment packets received
    packets_rx_++;

    // Account for possible linklayer padding
    packet->adjust_size_from_header();

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

    // Drop / forward if my ip address doesn't match dest. or broadcast
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

    PRINT("* Packet was for me (flags=%x)\n", (int) packet->ip_flags());

    // if the MF bit is set or fragment offset is non-zero, go to reassembly
    if (UNLIKELY(packet->ip_flags() == ip4::Flags::MF
              || packet->ip_frag_offs() != 0))
    {
      packet = this->reassemble(std::move(packet));
      if (packet == nullptr) return;
    }

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

    // Pass packet to it's respective protocol controller
    switch (packet->ip_protocol()) {
    case Protocol::ICMPv4:
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
      stack_.icmp().destination_unreachable(std::move(packet), icmp4::code::Dest_unreachable::PROTOCOL);

      drop(std::move(packet), Direction::Upstream, Drop_reason::Unknown_proto);
      break;
    }
  }

  void IP4::transmit(Packet_ptr pckt) {
    assert((size_t)pckt->size() > sizeof(header));

    auto packet = static_unique_ptr_cast<PacketIP4>(std::move(pckt));

    /*
     * RFC 1122 p. 30
     * When a host sends any datagram, the IP source address MUST
       be one of its own IP addresses (but not a broadcast or
       multicast address).
    */
    if (UNLIKELY(not stack_.is_valid_source(packet->ip_src()))) {
      drop(std::move(packet), Direction::Downstream, Drop_reason::Bad_source);
      return;
    }

    if (path_mtu_discovery_)
      packet->set_ip_flags(ip4::Flags::DF);

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

    ship(std::move(packet), 0, ct);
  }

  void IP4::ship(Packet_ptr pckt, addr next_hop, Conntrack::Entry_ptr ct)
  {
    auto packet = static_unique_ptr_cast<PacketIP4>(std::move(pckt));

    // Send loopback packets right back
    if (UNLIKELY(stack_.is_valid_source(packet->ip_dst()))) {
      PRINT("<IP4> Destination address is loopback \n");
      IP4::receive(std::move(packet), false);
      return;
    }

    // Filter illegal egress packets
    packet = drop_invalid_out(std::move(packet));
    if (packet == nullptr) return;

    /* POSTROUTING */
    auto& conntrack = stack_.conntrack();
    if(conntrack) {
      ct = (ct != nullptr) ?
        conntrack->confirm(ct->first, ct->proto) : conntrack->confirm(*packet);
    }
    auto res = postrouting_chain_(std::move(packet), stack_, ct);
    if (UNLIKELY(res == Filter_verdict_type::DROP)) return;

    Ensures(res.packet != nullptr);
    packet = res.release();

    if (next_hop == 0) {
      if (UNLIKELY(packet->ip_dst() == IP4::ADDR_BCAST)) {
        next_hop = IP4::ADDR_BCAST;
      }
      else {
        // Create local and target subnets
        addr target = packet->ip_dst()  & stack_.netmask();
        addr local  = stack_.ip_addr() & stack_.netmask();

        // Compare subnets to know where to send packet
        next_hop = target == local ? packet->ip_dst() : stack_.gateway();

        PRINT("<IP4 TOP> Next hop for %s, (netmask %s, local IP: %s, gateway: %s) == %s\n",
              packet->ip_dst().str().c_str(),
              stack_.netmask().str().c_str(),
              stack_.ip_addr().str().c_str(),
              stack_.gateway().str().c_str(),
              next_hop.str().c_str());

        if(UNLIKELY(next_hop == 0)) {
          PRINT("<IP4> Next_hop calculated to 0 (gateway == %s), dropping\n",
            stack_.gateway().str().c_str());
          drop(std::move(packet), Direction::Downstream, Drop_reason::Bad_destination);
          return;
        }
      }
    }

    // Stat increment packets transmitted
    packets_tx_++;

    PRINT("<IP4> Transmitting packet, layer begin: buf + %li ip.len=%u pkt.size=%zu\n",
      packet->layer_begin() - packet->buf(), packet->ip_total_length(), packet->size());

    linklayer_out_(std::move(packet), next_hop);
  }

  void IP4::set_path_mtu_discovery(bool on, uint16_t aged) noexcept {
    path_mtu_discovery_ = on;

    if (aged != 10)
      pmtu_aged_ = aged;

    if (not on and pmtu_timer_.is_running()) {
      pmtu_timer_.stop();
      paths_.clear();
    }
  }

  void IP4::update_path(Socket dest, PMTU new_pmtu, bool received_too_big,
    uint16_t total_length, uint8_t header_length) {

    if (UNLIKELY(not path_mtu_discovery_ or dest.address() == IP4::ADDR_ANY or (new_pmtu > 0 and new_pmtu < minimum_MTU())))
      return;

    if (UNLIKELY(dest.address().v4().is_multicast())) {
      // TODO RFC4821 p. 12

    }

    // If an entry for this destination already exists, update the Path MTU value, but only if
    // the value is smaller than the existing one
    auto it = paths_.find(dest);

    if (it != paths_.end()) {
      // If a router returns a next hop MTU value of zero: Try to discover the correct MTU value from the Total Length field
      if (UNLIKELY(new_pmtu == 0 and total_length not_eq 0))
        new_pmtu = pmtu_from_total_length(total_length, header_length, it->second.pmtu());

      if (new_pmtu < it->second.pmtu())
        it->second.set_pmtu(new_pmtu, received_too_big);
    } else {
      // If a router returns a next hop MTU value of zero: Try to discover the correct MTU value from the Total Length field
      if (UNLIKELY(new_pmtu == 0 and total_length not_eq 0))
        new_pmtu = pmtu_from_total_length(total_length, header_length, default_PMTU());

      // Add to paths_ if the entry doesn't exist
      // Initially, the PMTU value for a path is assumed to be the (known) MTU of the first-hop link
      // TODO PMTU: Maybe reset value according to the plateau table instead of default_PMTU()
      paths_.emplace(dest, PMTU_entry{new_pmtu, default_PMTU(), received_too_big});

      // Start the stale pmtu timer if it is not already running
      if (UNLIKELY(not pmtu_timer_.is_running()))
        pmtu_timer_.start(pmtu_timer_interval_);  // interval in seconds
    }
  }

  void IP4::remove_path(Socket dest) {
    auto it = paths_.find(dest);

    if (it != paths_.end())
      paths_.erase(it);
  }

  IP4::PMTU IP4::pmtu(Socket dest) const {
    auto it = paths_.find(dest);

    if (it != paths_.end())
      return it->second.pmtu();

    return 0;
  }

  RTC::timestamp_t IP4::pmtu_timestamp(Socket dest) const {
    auto it = paths_.find(dest);

    if (it != paths_.end())
      return it->second.timestamp();

    return 0;
  }

  IP4::PMTU IP4::pmtu_from_total_length(uint16_t total_length, uint8_t header_length, PMTU current) {
    /*
      RFC 1191 p. 8:
      Note: routers based on implementations derived from 4.2BSD Unix send an incorrect value
      for the Total Length of the original IP datagram. The value sent by these routers is the
      sum of the original Total Length and the original Header Length (expressed in octets).
      Since it is impossible for the host receiving such a Datagram Too Big message to know if it
      sent by one of these routers, the host must be conservative and assume that it is.
      If the Total Length field returned is not less than the current PMTU estimate, it must be
      reduced by 4 times the value of the returned Header Length field.
    */
    const uint16_t quad = header_length * 4;

    if (total_length >= current and total_length >= (quad + (PMTU) PMTU_plateau::ONE))
      total_length -= quad;

    return total_length;
  }

  void IP4::reset_stale_paths() {
    if (UNLIKELY(not path_mtu_discovery_)) {
      paths_.clear();

      if (pmtu_timer_.is_running())
        pmtu_timer_.stop();

      return;
    }

    if (UNLIKELY(pmtu_aged_ == PMTU_INFINITY)) {
      // Then the PMTU values should never be increased and there's no need for the timer
      pmtu_timer_.stop();
      return;
    }

    auto rtc_aged = (RTC::timestamp_t) pmtu_aged_;  // cast from uint16_t to int64_t (RTC::timestamp_t)
    rtc_aged = rtc_aged * 60; // from minutes to seconds

    for (auto it = paths_.begin(); it != paths_.end(); ++it) {
      /*
        If the timestamp of the PMTU_entry is not "reserved" and is older than the timout interval
        (default set to 10 minutes), then the PMTU can be increased/reset to see if the PMTU has
        increased since the last decrease over 10 minutes ago
      */

      if (it->second.timestamp() != 0 and RTC::time_since_boot() > rtc_aged and
        it->second.timestamp() < (RTC::time_since_boot() - rtc_aged))
      {
        stack_.reset_pmtu(it->first, it->second.pmtu());
        paths_.erase(it); // Optional, if keep the entry in the map: it->second.reset_pmtu();
      }
    }
  }

  uint16_t IP4::MDDS() const
  { return stack_.MTU() - sizeof(ip4::Header); }

  uint16_t IP4::default_PMTU() const noexcept
  { return stack_.MTU(); }

  const ip4::Addr IP4::local_ip() const {
    return stack_.ip_addr();
  }

} //< namespace net
