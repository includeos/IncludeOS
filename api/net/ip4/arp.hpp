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
#ifndef NET_IP4_ARP_HPP
#define NET_IP4_ARP_HPP

#include <rtc>
#include <unordered_map>

#include <delegate>
#include "ip4.hpp"

namespace net {

  class PacketArp;

  /** ARP manager, including an ARP-Cache. */
  class Arp {

  public:
    using Stack   = IP4::Stack;
    using Route_checker = delegate<bool(IP4::addr)>;
    /**
     *  You can assign your own ARP-resolution delegate
     *
     *  We're doing this to keep the Hårek Haugerud mapping (HH_MAP)
     */
    using Arp_resolver = delegate<void(Packet_ptr packet, IP4::addr)>;

    enum Opcode { H_request = 0x100, H_reply = 0x200 };

    /** Arp opcodes (Big-endian) */
    static constexpr uint16_t H_htype_eth {0x0100};
    static constexpr uint16_t H_ptype_ip4 {0x0008};
    static constexpr uint16_t H_hlen_plen {0x0406};

    /** Constructor */
    explicit Arp(Stack&) noexcept;

    struct __attribute__((packed)) header {
      uint16_t         htype;     // Hardware type
      uint16_t         ptype;     // Protocol type
      uint16_t         hlen_plen; // Protocol address length
      uint16_t         opcode;    // Opcode
      MAC::Addr     shwaddr;   // Source mac
      IP4::addr        sipaddr;   // Source ip
      MAC::Addr     dhwaddr;   // Target mac
      IP4::addr        dipaddr;   // Target ip
    };

    /** Handle incoming ARP packet. */
    void receive(Packet_ptr pckt);

    /** Roll your own arp-resolution system. */
    void set_resolver(Arp_resolver ar)
    { arp_resolver_ = ar; }

    enum Resolver_name { DEFAULT, HH_MAP };

    void set_resolver(Resolver_name nm) {
      switch (nm) {
      case HH_MAP:
        arp_resolver_ = {this, &Arp::hh_map};
        break;
      default:
        arp_resolver_ = {this, &Arp::arp_resolve};
      }
    }

    /** Add a route checker */
    void set_route_checker(Route_checker delg)
    { route_checker_ = delg; }

    /** Delegate link-layer output. */
    void set_linklayer_out(downstream_link link)
    { linklayer_out_ = link; }

    /** Downstream transmission. */
    void transmit(Packet_ptr, IP4::addr next_hop);

    /** Cache IP resolution. */
    void cache(IP4::addr, MAC::Addr);

    /** Flush the ARP cache */
    void flush_cache()
    { cache_.clear(); };

    /** Flush expired cache entries */
    void flush_expired () {
      for (auto ent : cache_) {
        if (ent.second.expired())
          cache_.erase(ent.first);
      }
    }

  private:

    /** ARP cache expires after cache_exp_t_ seconds */
    static constexpr uint16_t cache_exp_t_ {60 * 60 * 12};

    /** Cache entries are just MAC's and timestamps */
    struct Cache_entry {

      /** Map needs empty constructor (we have no emplace yet) */
      Cache_entry() noexcept = default;

      Cache_entry(MAC::Addr mac) noexcept
      : mac_(mac), timestamp_(RTC::time_since_boot()) {}

      Cache_entry(const Cache_entry& cpy) noexcept
      : mac_(cpy.mac_), timestamp_(cpy.timestamp_) {}

      bool expired() noexcept { return timestamp_ + cache_exp_t_ > RTC::time_since_boot(); }

      void update() noexcept { timestamp_ = RTC::time_since_boot(); }

      MAC::Addr mac()
      { return mac_; }

    private:
      MAC::Addr mac_;
      RTC::timestamp_t timestamp_;

    }; //< struct Cache_entry

    using Cache       = std::unordered_map<IP4::addr, Cache_entry>;
    using PacketQueue = std::unordered_map<IP4::addr, Packet_ptr>;


    /** Stats */
    uint32_t& requests_rx_;
    uint32_t& requests_tx_;
    uint32_t& replies_rx_;
    uint32_t& replies_tx_;

    Stack& inet_;
    Route_checker route_checker_ = nullptr;

    /** Needs to know which mac address to put in header->swhaddr */
    MAC::Addr mac_;

    /** Outbound data goes through here */
    downstream_link linklayer_out_ = nullptr;

    /** The ARP cache */
    Cache cache_;

    /** ARP resolution. */
    MAC::Addr resolve(IP4::addr);

    void arp_respond(header* hdr_in, IP4::addr ack_ip);

    // two different ARP resolvers
    void arp_resolve(Packet_ptr, IP4::addr next_hop);
    void hh_map(Packet_ptr, IP4::addr next_hop);

    Arp_resolver arp_resolver_ = {this, &Arp::arp_resolve};

    PacketQueue waiting_packets_;

    /** Add a packet to waiting queue, to be sent when IP is resolved */
    void await_resolution(Packet_ptr, IP4::addr);

    /** Create a default initialized ARP-packet */
    Packet_ptr create_packet();

  }; //< class Arp

} //< namespace net

#endif //< NET_ARP_HPP
