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

namespace net {

enum class Filter_verdict {
  ACCEPT,
  DROP
};

template <typename IPV>
struct Inet;

template <typename IPV>
using Packetfilter = delegate<Filter_verdict(typename IPV::IP_packet&, Inet<IPV>&)>;

template <typename IPV>
struct Filter_chain {
  std::list<Packetfilter<IPV>> chain;
  const char* name;

  Filter_verdict operator()(typename IPV::IP_packet& pckt, Inet<IPV>& stack) {
    auto verdict = Filter_verdict::ACCEPT;
    int i = 0;
    for (auto filter : chain) {
      i++;
      verdict = filter(pckt, stack);
      if(verdict == Filter_verdict::DROP) {
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
