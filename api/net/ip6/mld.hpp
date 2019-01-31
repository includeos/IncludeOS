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
#ifndef NET_IP6_MLD_HPP
#define NET_IP6_MLD_HPP

#include <rtc>
#include <unordered_map>
#include <deque>
#include <util/timer.hpp>
#include "packet_icmp6.hpp"
#include "packet_ndp.hpp"
#include <net/ip6/mld/message.hpp>

namespace net {
  class ICMPv6;

  class Mld {
  public:
    static const int        ROBUSTNESS_VAR              = 2;

    // Router constants
    static const int        QUERY_INTERVAL              = 125; // in seconds
    static const int        QUERY_RESPONSE_INTERVAL     = 10000; // in milliseconds
    static const int        MULTICAST_LISTENER_INTERVAL = (ROBUSTNESS_VAR *
        QUERY_INTERVAL * 1000) + QUERY_RESPONSE_INTERVAL; // in milliseconds
    static const int        OTHER_QUERIER_PRESENT_INTERVAL = (ROBUSTNESS_VAR *
        QUERY_INTERVAL * 1000) + (QUERY_RESPONSE_INTERVAL / 2); // in milliseconds
    static constexpr double STARTUP_QUERY_INTERVAL       = QUERY_INTERVAL / 4; // in seconds
    static const int        STARTUP_QUERY_COUNT          = ROBUSTNESS_VAR;
    static const int        LAST_LISTENER_QUERY_INTERVAL = 1000; // in milliseconds
    static const int        LAST_LISTENER_QUERY_COUNT    = ROBUSTNESS_VAR;

    // Node constants
    static const int        UNSOLICITED_REPORT_INTERVAL  = 10; // in seconds


    using Stack         = IP6::Stack;
    using ICMP_type     = ICMP6_error::ICMP_type;
    using State_handler = delegate<void(icmp6::Packet&)>;

    enum HostStates {
      NON_LISTENER,
      DELAYING_LISTENER,
      IDLE_LISTENER,
      MAX_HOST_STATE
    };

    enum RouterStates {
      QUERIER,
      NON_QUERIER,
      MAX_ROUTER_STATE
    };

    /** Constructor */
    explicit Mld(Stack&) noexcept;

    void receive(icmp6::Packet& pckt);
    void receive_query(icmp6::Packet& pckt);
    void send_report(const ip6::Addr& mcast);
    void mcast_expiry();

    void send_report_v2(const ip6::Addr& mcast);

    void receive(net::Packet_ptr pkt);
    void set_linklayer_out(downstream_link s)
    { linklayer_out_ = s; }

  private:
    void recv_query(icmp6::Packet& pckt);
    void recv_report(icmp6::Packet& pckt);
    void recv_done(icmp6::Packet& pckt);
    // MLDv2 RFC 3810
    void recv_query_v2(icmp6::Packet& pckt);
    void recv_report_v2(icmp6::Packet& pckt);

    void transmit(icmp6::Packet& pckt, MAC::Addr mac);

    struct MulticastHostNode {
    public:

      MulticastHostNode();

      void update_timestamp(const uint16_t ms)
      { timestamp_ = RTC::time_since_boot() + ms; }

      const ip6::Addr& addr() const { return mcast_addr_; }
      RTC::timestamp_t timestamp() const { return timestamp_; }

      bool expired() const noexcept
      { return RTC::time_since_boot() > timestamp_; }

      void handle(icmp6::Packet& pckt) { state_handlers_[state_](pckt); }

    private:
      const HostStates& state() const { return state_; }
      void setState(const HostStates state) { state_ = state; }
      void receive_query(const mld::Query& query);
      void non_listener_state_handler(icmp6::Packet& pckt);
      void delay_listener_state_handler(icmp6::Packet& pckt);
      void idle_listener_state_handler(icmp6::Packet& pckt);

      ip6::Addr             mcast_addr_;
      HostStates            state_;
      State_handler         state_handlers_[HostStates::MAX_HOST_STATE];
      RTC::timestamp_t      timestamp_;
    };

    struct MulticastRouterNode {
    public:
      MulticastRouterNode();
      const RouterStates& state() const { return state_; }
      void setState(const RouterStates state) { state_ = state; }

    private:
      ip6::Addr               mcast_addr_;
      RouterStates            state_;
      State_handler           state_handlers_[RouterStates::MAX_ROUTER_STATE];
      RTC::timestamp_t        timestamp_;
    };

    using RouterMlist = std::deque<MulticastRouterNode>;
    using HostMlist   = std::deque<MulticastHostNode>;

    struct Host {
    public:
      Host(Mld& ref) : mld_{ref} {}
      void receive(icmp6::Packet& pckt);
      void expiry();

    private:
      HostMlist mlist_;
      Mld       &mld_;
    };

    struct Router {
    public:
      Router(Mld& ref) : mld_{ref} {}
    private:
      [[maybe_unused]]Mld         &mld_;
      RouterMlist mlist_;
    };

    Stack& inet_;
    Timer  delay_timer_ {{ *this, &Mld::mcast_expiry }};
    Host   host_;
    Router router_;
    downstream_link linklayer_out_;
  };

  class Mld2 {
  public:
    static const int  ROBUSTNESS_VAR  = 2;
    enum class FilterMode : uint8_t {
      INCLUDE,
      EXCLUDE
    };
    using Stack      = IP6::Stack;
    using ICMP_type  = ICMP6_error::ICMP_type;
    using SourceList = std::deque<ip6::Addr>;

    /** Constructor */
    explicit Mld2(Stack&) noexcept;
    void receive(icmp6::Packet& pckt);
    void receive_query(icmp6::Packet& pckt);
    void receive_report(icmp6::Packet& pckt);
    bool allow(ip6::Addr source_addr, ip6::Addr mcast);
    void join(ip6::Addr addr, FilterMode filtermode, SourceList source_list);

  private:
    struct Multicast_listening_record {
    private:
      FilterMode filter_mode_;
      SourceList source_list_;
    public:
      Multicast_listening_record(FilterMode filter_mode, SourceList source_list) noexcept
        : filter_mode_{filter_mode}, source_list_{source_list} {}

      void exclude(SourceList source_list)
      {
        filter_mode_ = FilterMode::EXCLUDE;

        if (source_list_.empty()) {
          source_list_ = source_list;
          return;
        }

        for (auto src = source_list_.begin(); src != source_list_.end();)  {
          if (std::find(source_list.begin(), source_list.end(), *src) ==
              source_list.end()) {
            // source is not found in the new source list. Remove it
            src = source_list_.erase(src);
          }
        }
      }

      void include(SourceList source_list)
      {
        if (source_list_.empty()) {
          source_list_ = source_list;
        } else if (filter_mode_ == FilterMode::INCLUDE) {
          for(auto src : source_list) {
            if (std::find(source_list_.begin(), source_list_.end(), src) ==
                source_list_.end()) {
              source_list_.push_back(src);
            }
          }
        } else {
          for (auto src = source_list_.begin(); src != source_list_.end();)  {
            if (std::find(source_list.begin(), source_list.end(), *src) !=
                source_list.end()) {
              // source is found in the new source list. Remove it
              src = source_list_.erase(src);
            }
          }
        }
      }
    };

    using MldRec = std::unordered_map<ip6::Addr, Multicast_listening_record>;
    Stack& inet_;
    MldRec mld_records_;
  };
}
#endif //< NET_MLD_HPP
