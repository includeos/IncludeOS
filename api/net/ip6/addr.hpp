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
#ifndef NET_IP6_ADDR_HPP
#define NET_IP6_ADDR_HPP

#include <common>
#include <net/util.hpp>
#include <cstdlib>
#include <string>

namespace net {
namespace ip6 {

/**
 * This type is thrown when creating an instance of Addr
 * with a std::string that doesn't represent a valid IPv6
 * Address
 */
struct Invalid_Address : public std::runtime_error {
  using runtime_error::runtime_error;
}; //< struct Invalid_Address

/**
 * IPv6 Address representation
 */
struct Addr {
  Addr()
    : i32{{0, 0, 0, 0}} {}

  Addr(uint16_t a1, uint16_t a2, uint16_t b1, uint16_t b2,
       uint16_t c1, uint16_t c2, uint16_t d1, uint16_t d2)
  {
    i16[0] = htons(a1); i16[1] = htons(a2);
    i16[2] = htons(b1); i16[3] = htons(b2);
    i16[4] = htons(c1); i16[5] = htons(c2);
    i16[6] = htons(d1); i16[7] = htons(d2);
  }

  Addr(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
  {
    i32[0] = htonl(a); i32[1] = htonl(b);
    i32[2] = htonl(c); i32[3] = htonl(d);
  }

  Addr(const Addr& a)
  {
    for (int i = 0; i < 4; i++) {
        i32[i] = a.i32[i];
    }
  }

  // returns this IPv6 Address as a string
  std::string str() const {
    char ipv6_addr[40];
    snprintf(ipv6_addr, sizeof(ipv6_addr),
            "%0x:%0x:%0x:%0x:%0x:%0x:%0x:%0x",
            ntohs(i16[0]), ntohs(i16[1]), ntohs(i16[2]),
            ntohs(i16[3]), ntohs(i16[4]), ntohs(i16[5]),
            ntohs(i16[6]), ntohs(i16[7]));
    return ipv6_addr;
  }

  /**
   * Get a string representation of this type
   *
   * @return A string representation of this type
   */
  std::string to_string() const
  { return str(); }

  // multicast IPv6 Addresses
  static const Addr node_all_nodes;     // RFC 4921
  static const Addr node_all_routers;   // RFC 4921
  static const Addr node_mDNSv6;        // RFC 6762 (multicast DNSv6)

  // unspecified link-local Address
  static const Addr link_unspecified;

  // RFC 4291  2.4.6:
  // Link-Local Addresses are designed to be used for Addressing on a
  // neighbor discovery, or when no routers are present.
  // single link for purposes such as automatic Address configuration,
  static const Addr link_all_nodes;     // RFC 4921
  static const Addr link_all_routers;   // RFC 4921
  static const Addr link_mDNSv6;        // RFC 6762

  static const Addr link_dhcp_servers;  // RFC 3315
  static const Addr site_dhcp_servers;  // RFC 3315

  // returns true if this Addr is a IPv6 multicast Address
  bool is_multicast() const
  {
    /**
       RFC 4291 2.7 Multicast Addresses

       An IPv6 multicast Address is an identifier for a group of interfaces
       (typically on different nodes). An interface may belong to any
       number of multicast groups. Multicast Addresses have the following format:
       |   8    |  4 |  4 |                  112 bits                   |
       +------ -+----+----+---------------------------------------------+
       |11111111|flgs|scop|                  group ID                   |
       +--------+----+----+---------------------------------------------+
    **/
    return ((ntohs(i16[0]) & 0xFF00) == 0xFF00);
  }

  /**
   *
   **/
  bool is_loopback() const noexcept
  { return i32[0] == 0 && i32[1] == 0 &&
      i32[2] == 0 && ntohl(i32[3]) == 1; }

  /**
   * Assignment operator
   */
  Addr& operator=(const Addr other) noexcept {
    i32[0] = other.i32[0];
    i32[1] = other.i32[1];
    i32[2] = other.i32[2];
    i32[3] = other.i32[3];
    return *this;
  }

  /**
   * Operator to check for equality
   */
  bool operator==(const Addr other) const noexcept
  { return i32[0] == other.i32[0] && i32[1] == other.i32[1] &&
      i32[2] == other.i32[2] && i32[3] == other.i32[3]; }

  /**
   * Operator to check for inequality
   */
  bool operator!=(const Addr other) const noexcept
  { return not (*this == other); }

  /**
   * Operator to check for greater-than relationship
   */
  bool operator>(const Addr other) const noexcept
  {
      if (i32[0] > other.i32[0]) return true;
      if (i32[1] > other.i32[1]) return true;
      if (i32[2] > other.i32[2]) return true;
      if (i32[3] > other.i32[3]) return true;

      return false;
  }


  /**
   * Operator to check for greater-than-or-equal relationship
   */
  bool operator>=(const Addr other) const noexcept
  { return (*this > other or *this == other); }

  /**
   * Operator to check for lesser-than relationship
   */
  bool operator<(const Addr other) const noexcept
  { return not (*this >= other); }

  /**
   * Operator to check for lesser-than-or-equal relationship
   */
  bool operator<=(const Addr other) const noexcept
  { return (*this < other or *this == other); }

  /**
   * Operator to perform a bitwise-and operation on the given
   * IPv6 addresses
   */
  Addr operator&(const Addr other) const noexcept
  { return Addr{i32[0] & other.i32[0],
               i32[1] & other.i32[1],
               i32[2] & other.i32[2],
               i32[3] & other.i32[3] }; }

  Addr operator&(uint8_t prefix) const noexcept
  {
      int i = 3;
      uint8_t mask;
      uint32_t addr[32];

      addr[0] = htonl(i32[0]);
      addr[1] = htonl(i32[1]);
      addr[2] = htonl(i32[2]);
      addr[3] = htonl(i32[3]);

      if (prefix > 128) {
          prefix = 128;
      }

      mask = 128 - prefix;
      while (mask >= 32) {
          addr[i--] = 0;
          mask -= 32;
      }

      if (mask != 0) {
          addr[i] &= (0xFFFFFFFF << mask);
      }
      return Addr { addr[0], addr[1],
               addr[2], addr[3] };
  }

  Addr operator|(const Addr other) const noexcept
  { return Addr{i32[0] | other.i32[0],
               i32[1] | other.i32[1],
               i32[2] | other.i32[2],
               i32[3] | other.i32[3] }; }

  Addr operator~() const noexcept
  { return Addr{~i32[0], ~i32[1], ~i32[2], ~i32[3]}; }

  union
  {
    std::array<uint64_t, 2> i64;
    std::array<uint32_t ,4> i32;
    std::array<uint16_t, 8> i16;
  };
} __attribute__((packed)); //< struct Addr
} //< namespace ip6
} //< namespace net
#endif
