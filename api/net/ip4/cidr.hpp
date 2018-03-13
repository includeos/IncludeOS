// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 Oslo and Akershus University College of Applied Sciences
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

/**
 * This type is thrown when creating an instance of Cidr
 * with a std::string that doesn't represent a valid IPv4
 * cidr
 */
struct Invalid_cidr : std::runtime_error {
  using runtime_error::runtime_error;
}; //< struct Invalid_cidr

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
    Expects(mask <= 32);
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

  /**
   * @brief      Constructor
   *
   * Create an IPv4 cidr object to represent the cidr
   *
   * @param[in]  cidr  The cidr, f.ex. "10.0.0.0/24" ("10.0.0.5/24" is f.ex. also valid)
   */
  Cidr(const std::string& cidr)
  : Cidr{get_addr(cidr), get_mask(cidr)}
  {}

  constexpr bool contains(const Addr ip) const noexcept {
    return (ip >= from_) and (ip <= to_);
  }

  constexpr Addr from() const noexcept {
    return from_;
  }

  constexpr Addr to() const noexcept {
    return to_;
  }
private:
  constexpr Addr calc_from(const Addr address, const uint8_t mask) const noexcept {
    const uint32_t ip_mask = ntohl((0xFFFFFFFFUL << (32 - mask)) & 0xFFFFFFFFUL);
    return {address.whole & ip_mask};
  }

  constexpr Addr calc_to(const Addr address, const uint8_t mask) const noexcept {
    const uint32_t ip_mask = ntohl((0xFFFFFFFFUL << (32 - mask)) & 0xFFFFFFFFUL);
    return {(address.whole & ip_mask) | ~ip_mask};
  }

  Addr get_addr(const std::string& cidr) const {
    return {cidr.substr(0, cidr.find("/"))};
  }

  uint8_t get_mask(const std::string& cidr) const {
    const auto found = cidr.find('/');

    if (UNLIKELY(found == std::string::npos)) {
      throw Invalid_cidr{"Missing slash in " + cidr};
    }

    if (UNLIKELY(found >= (cidr.length() - 1))) {
      throw Invalid_cidr{"Missing mask after slash in " + cidr};
    }

    int val = 0;
    const char* mask = cidr.substr(found + 1).c_str(); // Getting past the '/'
    while (*mask) {
      Expects(std::isdigit(static_cast<int>(*mask)));
      val = (val * 10) + (*mask++ - '0');
    }

    Ensures((val >= 0) and (val <= 32));
    return static_cast<uint8_t>(val);
  }

  const Addr from_; //< Network address
  const Addr to_;   //< Broadcast address
}; //< class Cidr

} //< namespace ip4
} //< namespace net

#endif //< NET_IP4_CIDR_HPP
