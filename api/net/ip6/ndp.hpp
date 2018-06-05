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
#ifndef NET_IP6_NDP_HPP
#define NET_IP6_NDP_HPP

#include <rtc>
#include <unordered_map>
#include <util/timer.hpp>
#include "ip6.hpp"
#include "packet_icmp6.hpp"

using namespace std::chrono_literals;
namespace net {

  class ICMPv6;

  /** NDP manager, including an NDP-Cache. */
  class Ndp {

  public:

#define NEIGH_UPDATE_OVERRIDE          0x00000001
#define NEIGH_UPDATE_WEAK_OVERRIDE     0x00000002
#define NEIGH_UPDATE_OVERRIDE_ISROUTER 0x00000004
#define NEIGH_UPDATE_ISROUTER          0x40000000
#define NEIGH_UPDATE_ADMIN             0x80000000

    enum class NeighbourStates : uint8_t {
      INCOMPLETE,
      REACHABLE,
      STALE,
      DELAY,
      PROBE,
      FAIL
    };
    using Stack   = IP6::Stack;
    using Route_checker = delegate<bool(IP6::addr)>;
    using Ndp_resolver = delegate<void(IP6::addr)>;
    using ICMP_type = ICMP6_error::ICMP_type;

    /** Number of resolution retries **/
    static constexpr int ndp_retries = 3;

    /** Constructor */
    explicit Ndp(Stack&) noexcept;

    /** Handle incoming NDP packet. */
    void receive(icmp6::Packet& pckt);
    void receive_neighbour_solicitation(icmp6::Packet& pckt);
    void receive_neighbour_advertisement(icmp6::Packet& pckt);
    void receive_router_solicitation(icmp6::Packet& pckt);
    void receive_router_advertisement(icmp6::Packet& pckt);

    /** Send out NDP packet */
    void send_neighbour_solicitation(IP6::addr target);
    void send_neighbour_advertisement(icmp6::Packet& req);
    void send_router_solicitation();
    void send_router_advertisement();

    /** Roll your own ndp-resolution system. */
    void set_resolver(Ndp_resolver ar)
    { ndp_resolver_ = ar; }

    enum Resolver_name { DEFAULT, HH_MAP };

    /**
     * Set NDP proxy policy.
     * No route checker (default) implies NDP proxy functionality is disabled.
     *
     * @param delg : delegate to determine if we should reply to a given IP
     */
    void set_proxy_policy(Route_checker delg)
    { proxy_ = delg; }

    /** Downstream transmission. */
    void transmit(Packet_ptr, IP6::addr next_hop, MAC::Addr mac = MAC::EMPTY);

    /** Cache IP resolution. */
    void cache(IP6::addr ip, MAC::Addr mac, NeighbourStates state, uint32_t flags);
    void cache(IP6::addr ip, uint8_t *ll_addr, NeighbourStates state, uint32_t flags);

    /** Lookup for cache entry */
    bool lookup(IP6::addr ip);

    /** Flush the NDP cache */
    void flush_cache()
    { neighbour_cache_.clear(); };

    /** Flush expired cache entries */
    void flush_expired ();

    void set_neighbour_cache_flush_interval(std::chrono::minutes m) {
      flush_interval_ = m;
    }

    // Delegate output to link layer
    void set_linklayer_out(downstream_link s)
    { linklayer_out_ = s; }

    MAC::Addr& link_mac_addr()
    { return mac_; }

  private:

    /** NDP cache expires after neighbour_cache_exp_sec_ seconds */
    static constexpr uint16_t neighbour_cache_exp_sec_ {60 * 5};

    /** Cache entries are just MAC's and timestamps */
    struct Cache_entry {
      /** Map needs empty constructor (we have no emplace yet) */
      Cache_entry() noexcept = default;

      Cache_entry(MAC::Addr mac, NeighbourStates state, uint32_t flags) noexcept
      : mac_(mac), timestamp_(RTC::time_since_boot()), flags_(flags) {
          set_state(state);
      }

      Cache_entry(const Cache_entry& cpy) noexcept
      : mac_(cpy.mac_), state_(cpy.state_),
        timestamp_(cpy.timestamp_), flags_(cpy.flags_) {}

      void update() noexcept { timestamp_ = RTC::time_since_boot(); }

      bool expired() const noexcept
      { return RTC::time_since_boot() > timestamp_ + neighbour_cache_exp_sec_; }

      MAC::Addr mac() const noexcept
      { return mac_; }

      RTC::timestamp_t timestamp() const noexcept
      { return timestamp_; }

      RTC::timestamp_t expires() const noexcept
      { return timestamp_ + neighbour_cache_exp_sec_; }

      void set_state(NeighbourStates state)
      {
         if (state <= NeighbourStates::FAIL) {
           state_ = state;
         }
      }

      void set_flags(uint32_t flags)
      {
          flags_ = flags;
      }

      std::string get_state_name()
      {
          switch(state_) {
          case NeighbourStates::INCOMPLETE:
              return "incomplete";
          case NeighbourStates::REACHABLE:
              return "reachable";
          case NeighbourStates::STALE:
              return "stale";
          case  NeighbourStates::DELAY:
              return "delay";
          case NeighbourStates::PROBE:
              return "probe";
          case NeighbourStates::FAIL:
              return "fail";
          default:
              return "uknown";
          }
      }

    private:
      MAC::Addr        mac_;
      NeighbourStates  state_;
      RTC::timestamp_t timestamp_;
      uint32_t         flags_;
    }; //< struct Cache_entry

    struct Queue_entry {
      Packet_ptr pckt;
      int tries_remaining = ndp_retries;

      Queue_entry(Packet_ptr p)
        : pckt{std::move(p)}
      {}
    };

    using Cache       = std::unordered_map<IP6::addr, Cache_entry>;
    using PacketQueue = std::unordered_map<IP6::addr, Queue_entry>;

    /** Stats */
    uint32_t& requests_rx_;
    uint32_t& requests_tx_;
    uint32_t& replies_rx_;
    uint32_t& replies_tx_;

    std::chrono::minutes flush_interval_ = 5min;

    Timer resolve_timer_ {{ *this, &Ndp::resolve_waiting }};
    Timer flush_timer_ {{ *this, &Ndp::flush_expired }};

    Stack& inet_;
    Route_checker proxy_ = nullptr;

    MAC::Addr mac_;

    // Outbound data goes through here */
    downstream_link linklayer_out_ = nullptr;

    // The NDP cache
    Cache neighbour_cache_ {};

    // Packet queue
    PacketQueue waiting_packets_;

    // Settable resolver - defualts to ndp_resolve
    Ndp_resolver ndp_resolver_ = {this, &Ndp::ndp_resolve};

    /** Send an ndp resolution request */
    void ndp_resolve(IP6::addr next_hop);

    /**
     * Add a packet to waiting queue, to be sent when IP is resolved.
     */
    void await_resolution(Packet_ptr, IP6::addr);

    /** Create a default initialized NDP-packet */
    Packet_ptr create_packet();

    /** Retry ndp-resolution for packets still waiting */
    void resolve_waiting();

  }; //< class Ndp

} //< namespace net

#endif //< NET_NDP_HPP
