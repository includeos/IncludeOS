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
#include <util/timer.hpp>
#include "ip4.hpp"

using namespace std::chrono_literals;
namespace net {

  class PacketArp;

  /** ARP manager, including an ARP-Cache. */
  class Arp {

  public:
    using Stack   = IP4::Stack;
    using Route_checker = delegate<bool(ip4::Addr)>;
    using Arp_resolver = delegate<void(ip4::Addr)>;

    enum Opcode { H_request = 0x100, H_reply = 0x200 };

    /** Arp opcodes (Big-endian) */
    static constexpr uint16_t H_htype_eth {0x0100};
    static constexpr uint16_t H_ptype_ip4 {0x0008};
    static constexpr uint16_t H_hlen_plen {0x0406};

    /** Number of resolution retries **/
    static constexpr int arp_retries = 3;

    /** Constructor */
    explicit Arp(Stack&) noexcept;

    struct __attribute__((packed)) header {
      uint16_t         htype;     // Hardware type
      uint16_t         ptype;     // Protocol type
      uint16_t         hlen_plen; // Protocol address length
      uint16_t         opcode;    // Opcode
      MAC::Addr     shwaddr;   // Source mac
      ip4::Addr        sipaddr;   // Source ip
      MAC::Addr     dhwaddr;   // Target mac
      ip4::Addr        dipaddr;   // Target ip
    };

    /** Handle incoming ARP packet. */
    void receive(Packet_ptr pckt);

    /** Roll your own arp-resolution system. */
    void set_resolver(Arp_resolver ar)
    { arp_resolver_ = ar; }

    enum Resolver_name { DEFAULT, HH_MAP };

    /**
     * Set ARP proxy policy.
     * No route checker (default) implies ARP proxy functionality is disabled.
     *
     * @param delg : delegate to determine if we should reply to a given IP
     */
    void set_proxy_policy(Route_checker delg)
    { proxy_ = delg; }

    /** Delegate link-layer output. */
    void set_linklayer_out(downstream_link link)
    { linklayer_out_ = link; }

    /** Downstream transmission. */
    void transmit(Packet_ptr, ip4::Addr next_hop);

    /** Cache IP resolution. */
    void cache(ip4::Addr, MAC::Addr);

    /** Flush the ARP cache. RFC-2.3.2.1 */
    void flush_cache()
    { cache_.clear(); };

    /** Flush expired cache entries. RFC-2.3.2.1 */
    void flush_expired ();

    void set_cache_flush_interval(std::chrono::minutes m) {
      flush_interval_ = m;
    }

  private:

    /** ARP cache expires after cache_exp_sec_ seconds */
    static constexpr uint16_t cache_exp_sec_ {60 * 5};

    /** Cache entries are just MAC's and timestamps */
    struct Cache_entry {
      /** Map needs empty constructor (we have no emplace yet) */
      Cache_entry() noexcept = default;

      Cache_entry(MAC::Addr mac) noexcept
      : mac_(mac), timestamp_(RTC::time_since_boot()) {}

      Cache_entry(const Cache_entry& cpy) noexcept
      : mac_(cpy.mac_), timestamp_(cpy.timestamp_) {}

      void update() noexcept { timestamp_ = RTC::time_since_boot(); }

      bool expired() const noexcept
      { return RTC::time_since_boot() > timestamp_ + cache_exp_sec_; }

      MAC::Addr mac() const noexcept
      { return mac_; }

      RTC::timestamp_t timestamp() const noexcept
      { return timestamp_; }

      RTC::timestamp_t expires() const noexcept
      { return timestamp_ + cache_exp_sec_; }

    private:
      MAC::Addr mac_;
      RTC::timestamp_t timestamp_;
    }; //< struct Cache_entry

    struct Queue_entry {
      Packet_ptr pckt;
      int tries_remaining = arp_retries;

      Queue_entry(Packet_ptr p)
        : pckt{std::move(p)}
      {}
    };

    using Cache       = std::unordered_map<ip4::Addr, Cache_entry>;
    using PacketQueue = std::unordered_map<ip4::Addr, Queue_entry>;


    /** Stats */
    uint32_t& requests_rx_;
    uint32_t& requests_tx_;
    uint32_t& replies_rx_;
    uint32_t& replies_tx_;

    std::chrono::minutes flush_interval_ = 5min;

    Timer resolve_timer_ {{ *this, &Arp::resolve_waiting }};
    Timer flush_timer_ {{ *this, &Arp::flush_expired }};

    Stack& inet_;
    Route_checker proxy_ = nullptr;

    // Needs to know which mac address to put in header->swhaddr
    MAC::Addr mac_;

    // Outbound data goes through here */
    downstream_link linklayer_out_ = nullptr;

    // The ARP cache
    Cache cache_ {};

    // RFC-1122 2.3.2.2 Packet queue
    PacketQueue waiting_packets_;

    // Settable resolver - defualts to arp_resolve
    Arp_resolver arp_resolver_ = {this, &Arp::arp_resolve};

    /** Respond to arp request */
    void arp_respond(header* hdr_in, ip4::Addr ack_ip);

    /** Send an arp resolution request */
    void arp_resolve(ip4::Addr next_hop);

    /**
     * Add a packet to waiting queue, to be sent when IP is resolved.
     *
     * Implements RFC1122
     * 2.3.2.1 : Prevent ARP flooding
     * 2.3.2.2 : Packets SHOULD be queued.
     */
    void await_resolution(Packet_ptr, ip4::Addr);

    /** Create a default initialized ARP-packet */
    Packet_ptr create_packet();

    /** Retry arp-resolution for packets still waiting */
    void resolve_waiting();


  }; //< class Arp

} //< namespace net

#endif //< NET_ARP_HPP
