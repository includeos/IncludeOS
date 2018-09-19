// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#include <net/conntrack.hpp>

namespace net::tcp {

  Conntrack::Entry* tcp4_conntrack(Conntrack& ct, Quadruple q, const PacketIP4& pkt);

  enum class Ct_state : uint8_t
  {
    NONE,
    SYN_SENT,
    SYN_RECV,
    ESTABLISHED,
    FIN_WAIT,
    TIME_WAIT,
    CLOSE_WAIT,
    LAST_ACK,
    CLOSE
  };

  namespace timeout
  {
    static constexpr Conntrack::Timeout_duration ESTABLISHED {24*60*60};
    static constexpr Conntrack::Timeout_duration SYN_SENT    {2*60};
    static constexpr Conntrack::Timeout_duration SYN_RECV    {60};
    static constexpr Conntrack::Timeout_duration FIN_WAIT    {2*60};
    static constexpr Conntrack::Timeout_duration TIME_WAIT   {2*60};
    static constexpr Conntrack::Timeout_duration CLOSE_WAIT  {12*60};
    static constexpr Conntrack::Timeout_duration LAST_ACK    {30};

    static constexpr Conntrack::Timeout_duration NONE        {30*60};
    static constexpr Conntrack::Timeout_duration CLOSE       {10};
  }

}
