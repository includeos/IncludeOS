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
#ifndef NET_NETFILTER_HPP
#define NET_NETFILTER_HPP

#include <delegate>
#include <list>
#include "conntrack.hpp"

namespace net {

enum class Filter_verdict_type : uint8_t {
  ACCEPT,
  DROP
};

/**
 * @brief      A verdict returned from a filter on what to do
 *             with the containing packet.
 *
 * @tparam     IPV   IP Version (4 or 6)
 */
template <typename IPV>
struct Filter_verdict
{
  using IP_packet_ptr = typename IPV::IP_packet_ptr;

  Filter_verdict(IP_packet_ptr pkt, const Filter_verdict_type verd) noexcept
    : packet{std::move(pkt)}, verdict{verd}
  {}

  Filter_verdict() noexcept
    : Filter_verdict(nullptr, Filter_verdict_type::DROP)
  {}

  IP_packet_ptr       packet;
  Filter_verdict_type verdict;

  operator Filter_verdict_type() const noexcept
  { return verdict; }

  /**
   * @brief      Release the packet, giving away ownership.
   *
   * @return     The packet ptr
   */
  IP_packet_ptr release()
  { return std::move(packet); }
};

class Inet;

template <typename IPV>
using Packetfilter =
  delegate<Filter_verdict<IPV>(typename IPV::IP_packet_ptr, Inet&, Conntrack::Entry_ptr)>;

/**
 * @brief      A filter chain consisting of a list of packet filters.
 *
 * @tparam     IPV   IP Version (4 or 6)
 */
template <typename IPV>
struct Filter_chain
{
  using IP_packet_ptr = typename IPV::IP_packet_ptr;

  std::list<Packetfilter<IPV>> chain;
  const char* name;

  /**
   *  Execute the chain
   */
  Filter_verdict<IPV> operator()(IP_packet_ptr pckt, Inet& stack, Conntrack::Entry_ptr ct)
  {
    Filter_verdict<IPV> verdict{std::move(pckt), Filter_verdict_type::ACCEPT};
    int i = 0;
    for (auto& filter : chain) {
      i++;
      verdict = filter(verdict.release(), stack, ct);
      if(verdict == Filter_verdict_type::DROP) {
        debug("Packet dropped in %s chain, filter %i \n", name, i);
        break;
      }
    }
    return verdict;
  }

  Filter_chain(const char* chain_name, std::initializer_list<Packetfilter<IPV>> filters)
    : chain(filters), name{chain_name}
  {}
};

}

#endif
