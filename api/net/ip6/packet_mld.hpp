// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef PACKET_MLD_HPP
#define PACKET_MLD_HPP

#include <net/ip6/ip6.hpp>
#include <net/ip6/icmp6_error.hpp>
#include <cstdint>
#include <gsl/span>

namespace net::icmp6 {
  class Packet;
}

namespace net::mld {

  class MldPacket2 {
  private:
    icmp6::Packet&  icmp6_;

  public:
    struct Query {
      ip6::Addr multicast;
      uint8_t   reserved : 4,
                supress  : 1,
                qrc      : 3;
      uint8_t   qqic;
      uint16_t  num_srcs;
      ip6::Addr sources[0];
    };

    struct multicast_address_rec {
      uint8_t   rec_type;
      uint8_t   data_len;
      uint16_t  num_src;
      ip6::Addr multicast;
      ip6::Addr sources[0];
    };

    struct Report {
      multicast_address_rec records[0];
    };

    MldPacket2(icmp6::Packet& icmp6);
    Query& query();
    Report& report();
  };
}
#endif
