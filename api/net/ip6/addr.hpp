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
#include <hw/mac_addr.hpp>

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
  constexpr Addr() noexcept
    : i32{{0, 0, 0, 0}} {}

  Addr(uint16_t a1, uint16_t a2, uint16_t b1, uint16_t b2,
       uint16_t c1, uint16_t c2, uint16_t d1, uint16_t d2)
    : i16{htons(a1), htons(a2), htons(b1), htons(b2),
          htons(c1), htons(c2), htons(d1), htons(d2)}
  {}

  explicit Addr(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
    : i32{htonl(a), htonl(b), htonl(c), htonl(d)}
  {}

  explicit Addr(uint64_t a, uint64_t b)
    : i64{htonll(a), htonll(b)}
  {}

  Addr(const Addr& a) noexcept
    : i64{a.i64} {}

  Addr(Addr&& a) noexcept
    : i64{a.i64} {}

  /**
   * Constructor
   *
   * Construct an IPv6 address from a {std::string} object
   * representing an IPv6 address
   *
   * @param addr
   * A {std::string} object representing an IPv6 address
   *
   * @throws Invalid_address
   *  IIf the {std::string} object doesn't representing a valid IPv6
   *  address
   */
  Addr(const std::string &addr);

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
  static const Addr addr_any;

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
  bool is_multicast() const noexcept
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

  bool is_linklocal() const noexcept
  {
    return ((ntohs(i16[0]) & 0xFE80) == 0xFE80);
  }

  bool is_solicit_multicast() const noexcept
  {
    return i32[0] == htonl(0xFF020000)
       and i32[1] == 0
       and i32[2] == htonl(0x1)
       and (i32[3] & htonl(0xFF000000)) == htonl(0xFF000000);
  }

  uint8_t* data()
  {
      return reinterpret_cast<uint8_t*> (i16.data());
  }

  static Addr solicit(const Addr& other) noexcept
  {
    return Addr{0xFF020000, 0, 0x1, (0xFF000000 | ntohl(other.i32[3]))};
  }

  static Addr link_local(uint64_t eui) noexcept
  {
    return Addr{0xFE80'0000'0000'0000, ntohll(eui)};
  }

  /**
   *
   **/
  bool is_loopback() const noexcept
  { return i32[0] == 0 && i32[1] == 0 &&
      i32[2] == 0 && ntohl(i32[3]) == 1; }

  template <typename T>
  T get_part(const uint8_t n) const
  {
     static_assert(std::is_same_v<T, uint8_t> or
             std::is_same_v<T, uint16_t> or
             std::is_same_v<T, uint32_t> or
             std::is_same_v<T, uint64_t>, "Unallowed T");

     if constexpr (std::is_same_v<T, uint8_t>) {
         Expects(n < 16);
         return i8[n];
     } else if constexpr (std::is_same_v<T, uint16_t>) {
         Expects(n < 8);
         return i16[n];
     } else if constexpr (std::is_same_v<T, uint32_t>) {
         Expects(n < 4);
         return i32[n];
     } else {
         Expects(n < 2);
         return i64[n];
     }
  }

  template <typename T>
  void set_part(const uint8_t n, T val)
  {
     static_assert(std::is_same_v<T, uint8_t> or
             std::is_same_v<T, uint16_t> or
             std::is_same_v<T, uint32_t> or
             std::is_same_v<T, uint64_t>, "Unallowed T");

     if constexpr (std::is_same_v<T, uint8_t>) {
         Expects(n < 16);
         i8[n] = val;
     } else if constexpr (std::is_same_v<T, uint16_t>) {
         Expects(n < 8);
         i16[n] = val;
     } else if constexpr (std::is_same_v<T, uint32_t>) {
         Expects(n < 4);
         i32[n] = val;
     } else {
         Expects(n < 2);
         i64[n] = val;
     }
  }

  void set(const MAC::Addr &mac, const uint8_t loc = 0)
  {
      Expects(loc <= (16 - 6));
      uint8_t start_loc = (10 - loc);

      for (int i = 0; i < 6; i++) {
        i8[start_loc++] = mac[i];
      }
  }

  /**
   * Assignment operator
   */
  Addr& operator=(const Addr& other) noexcept
  {
    i64 = other.i64;
    return *this;
  }

  Addr& operator=(Addr&& other) noexcept
  {
    i64 = other.i64;
    return *this;
  }

  /**
   * Operator to check for equality
   */
  bool operator==(const Addr& other) const noexcept
  { return i64 == other.i64; }

  /**
   * Operator to check for inequality
   */
  bool operator!=(const Addr& other) const noexcept
  { return not (*this == other); }

  /**
   * Operator to check for lesser-than relationship
   */
  bool operator<(const Addr& other) const noexcept
  { return i32 < other.i32; }

  /**
   * Operator to check for greater-than relationship
   */
  bool operator>(const Addr& other) const noexcept
  { return i32 > other.i32; }

  /**
   * Operator to check for lesser-than-or-equal relationship
   */
  bool operator<=(const Addr& other) const noexcept
  { return (*this < other or *this == other); }

  /**
   * Operator to check for greater-than-or-equal relationship
   */
  bool operator>=(const Addr& other) const noexcept
  { return (*this > other or *this == other); }

  /**
   * Operator to perform a bitwise-and operation on the given
   * IPv6 addresses
   */
  Addr operator&(const Addr& other) const noexcept
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

  Addr operator|(const Addr& other) const noexcept
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
    std::array<uint8_t, 16> i8;
  };
} __attribute__((packed)); //< struct Addr
static_assert(sizeof(Addr) == 16, "Must be 16 bytes in size");

} //< namespace ip6
} //< namespace net

// Allow an IPv6 address to be used as key in e.g. std::unordered_map
namespace std {
  template<>
  struct hash<net::ip6::Addr> {
    size_t operator()(const net::ip6::Addr& addr) const {
      // This is temporary. Use a proper hash
      return std::hash<uint64_t>{}(addr.i64[0] + addr.i64[1]);
    }
  };
} //< namespace std
#endif
