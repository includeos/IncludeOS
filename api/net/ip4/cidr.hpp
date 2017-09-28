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

#ifndef NET_IP4_CIDR_HPP
#define NET_IP4_CIDR_HPP

#include "addr.hpp"

namespace net {
namespace ip4 {

class Cidr {
public:
  Cidr(uint8_t p1, uint8_t p2, uint8_t p3, uint8_t p4, uint8_t mask)
  {
    uint32_t ip_whole = Addr{p1,p2,p3,p4}.whole;
    uint32_t ip_mask = net::ntohl((0xFFFFFFFFUL << (32 - mask)) & 0xFFFFFFFFUL);

    uint32_t from_ip = ip_whole & ip_mask;
    uint32_t to_ip = from_ip | ~ip_mask;

    from_ = Addr{from_ip};
    to_ = Addr{to_ip};
  }

  // Possibly not necessary
  Cidr(Addr from, Addr to)
  : from_{from}, to_{to}
  {}

  bool contains(Addr ip) {
    return ip >= from_ and ip <= to_;
  }

  Addr from() {
    return from_;
  }

  Addr to() {
    return to_;
  }

private:
  Addr from_; // network address
  Addr to_;   // broadcast address
}; //< class Cidr

} //< namespace ip4
} //< namespace net

#endif
