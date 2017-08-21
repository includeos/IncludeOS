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
  using Lookup_table = std::map<Quintuple, Entry*>;

  /**
   * @brief      The state of the connection.
   */
  enum class State : uint8_t {
    NEW,
    ESTABLISHED
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
    Quadruple         out;
    Quadruple         in;
    RTC::timestamp_t  timeout;
    Protocol          proto;
    State             state;
    Seen              direction;

    Entry(Quadruple o, Protocol p, Seen d)
      : out(std::move(o)), in({out.dst, out.src}),
        proto(p), state(State::NEW), direction(d)
    {}

    bool is_mirrored() const noexcept
    { return out.src == in.dst and out.dst == in.src; }
  };

public:

  /**
   * @brief      Find the entry where the outgoing quadruple
   *             with the given protocol matches.
   *
   * @param[in]  quad   The quad
   * @param[in]  proto  The prototype
   *
   * @return     A matching conntrack entry (nullptr if not found)
   */
  Entry* out(const Quadruple& quad, const Protocol proto) const;

  /**
   * @brief      Find the entry where the incoming quadruple
   *             with the given protocol matches.
   *
   * @param[in]  quad   The quad
   * @param[in]  proto  The prototype
   *
   * @return     A matching conntrack entry (nullptr if not found)
   */
  Entry* in(const Quadruple& quad, const Protocol proto) const;

  /**
   * @brief      Track a outgoing packet, updating the state of the entry.
   *
   * @param[in]  pkt   The packet
   *
   * @return     The conntrack entry related to this packet.
   */
  Entry* track_out(const PacketIP4& pkt);

  /**
   * @brief      Track a incoming packet, updating the state of the entry.
   *
   * @param[in]  pkt   The packet
   *
   * @return     The conntrack entry related to this packet.
   */
  Entry* track_in(const PacketIP4& pkt);

  /**
   * @brief      Adds an entry, mirroring the outgoing quadruple.
   *
   * @param[in]  quad   The outgoing quadruple
   * @param[in]  proto  The prototype
   * @param[in]  dir    The direction the packet is going
   *
   * @return     { description_of_the_return_value }
   */
  Entry* add_entry(const Quadruple& quad, const Protocol proto, const Seen dir);

  void remove_entry(Entry*);

  /**
   * @brief      A very simple and unreliable way for tracking outgoing quintuples.
   *
   * @param[in]  quad   The quad
   * @param[in]  proto  The prototype
   *
   * @return     The conntrack entry related to quintuple.
   */
  Entry* simple_track_out(Quadruple quad, const Protocol proto);

  /**
   * @brief      A very simple and unreliable way for tracking incoming quintuples.
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

  Conntrack();

  using Timeout_duration = std::chrono::seconds;
  Timeout_duration timeout_new      {30};
  Timeout_duration timeout_est      {180};
  Timeout_duration timeout_new_tcp  {60};
  Timeout_duration timeout_est_tcp  {600};

  Packet_tracker tcp_out;
  Packet_tracker tcp_in;

  Entry_handler on_close; // single for now

private:
  std::vector<std::unique_ptr<Entry>> entries;
  Lookup_table out_lookup;
  Lookup_table in_lookup;

};

}

#endif
