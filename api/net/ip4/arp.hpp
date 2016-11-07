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

#include <os>
#include <map>

#include <delegate>
#include "ip4.hpp"

namespace net {

  class PacketArp;

  /** ARP manager, including an ARP-Cache. */
  class Arp {
    using Stack   = IP4::Stack;
  private:
    /** ARP cache expires after cache_exp_t_ seconds */
    static constexpr uint16_t cache_exp_t_ {60 * 60 * 12};

    /** Cache entries are just MAC's and timestamps */
    struct cache_entry {
      Ethernet::addr mac_;
      uint64_t       timestamp_;

      /** Map needs empty constructor (we have no emplace yet) */
      cache_entry() noexcept = default;

      cache_entry(Ethernet::addr mac) noexcept
      : mac_(mac), timestamp_(OS::uptime()) {}

      cache_entry(const cache_entry& cpy) noexcept
      : mac_(cpy.mac_), timestamp_(cpy.timestamp_) {}

      void update() noexcept { timestamp_ = OS::uptime(); }
    }; //< struct cache_entry

    using Cache       = std::map<IP4::addr, cache_entry>;
    using PacketQueue = std::map<IP4::addr, Packet_ptr>;
  public:
    /**
     *  You can assign your own ARP-resolution delegate
     *
     *  We're doing this to keep the Hårek Haugerud mapping (HH_MAP)
     */
    using Arp_resolver = delegate<void(Packet_ptr packet)>;

    enum Opcode { H_request = 0x100, H_reply = 0x200 };

    /** Arp opcodes (Big-endian) */
    static constexpr uint16_t H_htype_eth {0x0100};
    static constexpr uint16_t H_ptype_ip4 {0x0008};
    static constexpr uint16_t H_hlen_plen {0x0406};

    /** Constructor */
    explicit Arp(Stack&) noexcept;

    struct __attribute__((packed)) header {
      Ethernet::header ethhdr;    // Ethernet header
      uint16_t         htype;     // Hardware type
      uint16_t         ptype;     // Protocol type
      uint16_t         hlen_plen; // Protocol address length
      uint16_t         opcode;    // Opcode
      Ethernet::addr   shwaddr;   // Source mac
      IP4::addr        sipaddr;   // Source ip
      Ethernet::addr   dhwaddr;   // Target mac
      IP4::addr        dipaddr;   // Target ip
    };

    /** Handle incoming ARP packet. */
    void bottom(Packet_ptr pckt);

    /** Roll your own arp-resolution system. */
    void set_resolver(Arp_resolver ar)
    { arp_resolver_ = ar; }

    enum Resolver_name { DEFAULT, HH_MAP };

    void set_resolver(Resolver_name nm) {
      // @TODO: Add HÅREK-mapping here
      switch (nm) {
      case HH_MAP:
        arp_resolver_ = {this, &Arp::hh_map};
        break;
      default:
        arp_resolver_ = {this, &Arp::arp_resolve};
      }
    }

    /** Delegate link-layer output. */
    void set_linklayer_out(downstream link)
    { linklayer_out_ = link; }

    /** Downstream transmission. */
    void transmit(Packet_ptr);

  private:

    /** Stats */
    uint32_t& requests_rx_;
    uint32_t& requests_tx_;
    uint32_t& replies_rx_;
    uint32_t& replies_tx_;

    Stack& inet_;

    /** Needs to know which mac address to put in header->swhaddr */
    Ethernet::addr mac_;

    /** Outbound data goes through here */
    downstream linklayer_out_;

    /** The ARP cache */
    Cache cache_;

    /** Cache IP resolution. */
    void cache(IP4::addr, Ethernet::addr);

    /** Check if an IP is cached and not expired */
    bool is_valid_cached(IP4::addr);

    /** ARP resolution. */
    Ethernet::addr resolve(IP4::addr);

    void arp_respond(header* hdr_in);

    // two different ARP resolvers
    void arp_resolve(Packet_ptr);
    void hh_map(Packet_ptr);

    Arp_resolver arp_resolver_ = {this, &Arp::arp_resolve};

    PacketQueue waiting_packets_;

    /** Add a packet to waiting queue, to be sent when IP is resolved */
    void await_resolution(Packet_ptr, IP4::addr);

    /** Create a default initialized ARP-packet */
    Packet_ptr create_packet();
  }; //< class Arp

} //< namespace net

#endif //< NET_ARP_HPP
