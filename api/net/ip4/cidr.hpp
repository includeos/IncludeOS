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

  /**
   * @brief      Constructor
   *
   * Create an IPv4 cidr object to represent the cidr <address/mask>,
   * f.ex. 10.0.0.0/24
   *
   * @param[in]  address  IPv4 address
   * @param[in]  mask     A number between 0 and 32, representing the number
   *                        of leading 1 bits in the netmask, f.ex.:
   *                        32 represents the netmask 255.255.255.255
   *                        24 represents the netmask 255.255.255.0
   */
  constexpr Cidr(const Addr address, const uint8_t mask) noexcept
  : from_{calc_from(address, mask)}, to_{calc_to(address, mask)}
  {
    Expects(mask >= 0 and mask <= 32);
  }

  /**
   * @brief      Constructor
   *
   * Create an IPv4 cidr object to represent the cidr <p1.p2.p3.p4/mask>
   *
   * @param[in]  p1    The first part of the IPv4 cidr
   * @param[in]  p2    The second part of the IPv4 cidr
   * @param[in]  p3    The third part of the IPv4 cidr
   * @param[in]  p4    The fourth part of the IPv4 cidr
   * @param[in]  mask  A number between 0 and 32, representing the number
   *                    of leading 1 bits in the netmask, f.ex.:
   *                    32 represents the netmask 255.255.255.255
   *                    24 represents the netmask 255.255.255.0
   */
  constexpr Cidr(const uint8_t p1, const uint8_t p2, const uint8_t p3, const uint8_t p4,
    const uint8_t mask) noexcept
  : Cidr{{p1,p2,p3,p4}, mask}
  {}

  bool contains(Addr ip) const noexcept {
    return ip >= from_ and ip <= to_;
  }

  Addr from() const noexcept {
    return from_;
  }

  Addr to() const noexcept {
    return to_;
  }

private:
  constexpr Addr calc_from(const Addr address, const uint8_t mask) {
    const uint32_t ip_mask = net::ntohl((0xFFFFFFFFUL << (32 - mask)) & 0xFFFFFFFFUL);
    return {address.whole & ip_mask};
  }

  constexpr Addr calc_to(const Addr address, const uint8_t mask) {
    const uint32_t ip_mask = net::ntohl((0xFFFFFFFFUL << (32 - mask)) & 0xFFFFFFFFUL);
    const uint32_t from_ip = address.whole & ip_mask;
    return {from_ip | ~ip_mask};
  }

  const Addr from_; // network address
  const Addr to_;   // broadcast address
}; //< class Cidr

} //< namespace ip4
} //< namespace net

#endif
