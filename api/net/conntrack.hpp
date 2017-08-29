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
#ifndef NET_CONNTRACK_HPP
#define NET_CONNTRACK_HPP

#include <net/socket.hpp>
#include <net/ip4/packet_ip4.hpp>
#include <vector>
#include <map>
#include <rtc>
#include <chrono>
#include "netfilter.hpp"

namespace net {

class Conntrack {
public:
  struct Entry;
  /**
   * Custom handler for tracking packets in a certain way
   */
  using Packet_tracker = delegate<Entry*(Conntrack&, Quadruple, const PacketIP4&)>;

  using Entry_handler = delegate<void(Entry*)>;

  /**
   * @brief      Key for lookup tables
   */
  struct Quintuple {
    Quadruple quad;
    Protocol  proto;

    Quintuple(Quadruple q, const Protocol p)
      : quad(std::move(q)), proto(p)
    {}

    bool operator==(const Quintuple& other) const noexcept
    { return proto == other.proto and quad == other.quad; }

    bool operator<(const Quintuple& other) const noexcept {
      return proto < other.proto
        or (proto == other.proto and quad < other.quad);
    }
  };

  /**
   * @brief      The state of the connection.
   */
  enum class State : uint8_t {
    NEW,
    ESTABLISHED,
    RELATED
  };

  /**
   * @brief      Which direction packet has been seen on a connection.
   */
  enum class Seen : uint8_t {
    OUT, IN, BOTH
  };

  /**
   * @brief      A entry in the connection tracker (a Connection)
   */
  struct Entry {
    Quadruple         first;
    Quadruple         second;
    RTC::timestamp_t  timeout;
    Protocol          proto;
    State             state;

    Entry(Quadruple quad, Protocol p)
      : first{std::move(quad)}, second{first.dst, first.src},
        proto(p), state(State::NEW)
    {}

    bool is_mirrored() const noexcept
    { return first.src == second.dst and first.dst == second.src; }

    std::string to_string() const;

  };

public:

  /**
   * @brief      Find the entry where the quadruple
   *             with the given protocol matches.
   *
   * @param[in]  quad   The quad
   * @param[in]  proto  The prototype
   *
   * @return     A matching conntrack entry (nullptr if not found)
   */
  Entry* get(const Quadruple& quad, const Protocol proto) const;

  /**
   * @brief      Track a packet, updating the state of the entry.
   *
   * @param[in]  pkt   The packet
   *
   * @return     The conntrack entry related to this packet.
   */
  Entry* in(const PacketIP4& pkt);

  /**
   * @brief      Confirms a connection, moving the entry to confirmed.
   *
   * @param[in]  pkt   The packet
   *
   * @return     { description_of_the_return_value }
   */
  Entry* confirm(const PacketIP4& pkt);

  /**
   * @brief      Adds an entry, mirroring the quadruple.
   *
   * @param[in]  quad   The quadruple
   * @param[in]  proto  The prototype
   * @param[in]  dir    The direction the packet is going
   *
   * @return     { description_of_the_return_value }
   */
  Entry* add_entry(const Quadruple& quad, const Protocol proto);

  /**
   * @brief      Update one quadruple of a old entry (proto + oldq)
   *             to a new Quadruple. This changes the entry and updates the key.
   *
   * @param[in]  proto  The protocol
   * @param[in]  oldq   The old (current) quadruple
   * @param[in]  newq   The new quadruple
   */
  void update_entry(const Protocol proto, const Quadruple& oldq, const Quadruple& newq);

  /**
   * @brief      Flush expired entries.
   */
  void flush_expired();

  void remove_entry(Entry* entry);

  /**
   * @brief      A very simple and unreliable way for tracking quintuples.
   *
   * @param[in]  quad   The quad
   * @param[in]  proto  The prototype
   *
   * @return     The conntrack entry related to quintuple.
   */
  Entry* simple_track_in(Quadruple quad, const Protocol proto);

  /**
   * @brief      Gets the quadruple from a IP4 packet.
   *             Assumes the packet has protocol specific payload.
   *
   * @param[in]  pkt   The packet
   *
   * @return     The quadruple.
   */
  static Quadruple get_quadruple(const PacketIP4& pkt);

  static Quadruple get_quadruple_icmp(const PacketIP4& pkt);

  template<typename IPV>
  Packetfilter<IPV> in_filter() {
    return [this] (PacketIP4& pkt, Inet<IPV>&)->auto {
      return (in(pkt) != nullptr)
        ? Filter_verdict::ACCEPT : Filter_verdict::DROP;
    };
  }

  template<typename IPV>
  Packetfilter<IPV> confirm_filter() {
    return [this] (PacketIP4& pkt, Inet<IPV>&)->auto {
      confirm(pkt);
      return Filter_verdict::ACCEPT; // always accept?
    };
  }

  Conntrack();

  using Timeout_duration = std::chrono::seconds;
  Timeout_duration gc_interval_     {10};

  Timeout_duration timeout_new      {30};
  Timeout_duration timeout_est      {180};
  Timeout_duration timeout_new_tcp  {60};
  Timeout_duration timeout_est_tcp  {600};

  Packet_tracker tcp_in;

  Entry_handler on_close; // single for now

private:
  using Entry_table = std::map<Quintuple, std::shared_ptr<Entry>>;
  Entry_table entries;
  Entry_table unconfirmed;
  //Timer       flush_timer_;

  void update_timeout(Entry& ent, Timeout_duration dur);

};

}

#endif
