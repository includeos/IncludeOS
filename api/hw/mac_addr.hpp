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
#ifndef HW_MAC_ADDR_HPP
#define HW_MAC_ADDR_HPP

#include <common>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

namespace MAC {

/**
 * MAC address representation
 */
union Addr {
  /**
   * Default constructor
   */
  constexpr Addr() noexcept : part{} {}

  /**
   * Constructor
   *
   * Create a MAC address by specifying each part of the address
   *
   * @param a
   *  The first part of the MAC address
   *
   * @param b
   *  The second part of the MAC address
   *
   * @param c
   *  The third part of the MAC address
   *
   * @param d
   *  The fourth part of the MAC address
   *
   * @param e
   *  The fifth part of the MAC address
   *
   * @param f
   *  The sixth part of the MAC address
   */
  constexpr Addr(const uint8_t a, const uint8_t b, const uint8_t c,
                 const uint8_t d, const uint8_t e, const uint8_t f) noexcept
    : part{a,b,c,d,e,f}
  {}

  static uint8_t dehex(char c)
  {
    if (c >= '0' && c <= '9')
        return (c - '0');
    else if (c >= 'a' && c <= 'f')
        return 10 + (c - 'a');
    else if (c >= 'A' && c <= 'F')
        return 10 + (c - 'A');
    else
        return 0;
  }

  Addr(const std::string& smac) noexcept
    : Addr(smac.c_str())
  {}

  Addr(const char *smac) noexcept
  {
    uint8_t macaddr[PARTS_LEN] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
    for (size_t i = 0; i < PARTS_LEN; i++) {
        macaddr[i] = dehex(*smac++) << 4;
        macaddr[i] |= dehex(*smac++);
        smac++;
    }
    memcpy(part, macaddr, PARTS_LEN);
  }

  Addr(uint8_t *addr) noexcept
  {
      if (addr) {
        memcpy(part, addr, PARTS_LEN);
      }
  }

  /**
   * Assignment operator
   *
   * @param other
   *  The MAC address object to assign from
   *
   * @return The object that invoked this method
   */
  constexpr Addr& operator=(const Addr other) noexcept {
    minor = other.minor;
    major = other.major;
    return *this;
  }


  /**
   * Get a hex string representation of a MAC address
   *
   * @return A hex string representation of a MAC address
   */
  std::string hex_str() const {
    char hex_addr[18];

    snprintf(hex_addr, sizeof(hex_addr), "%02x:%02x:%02x:%02x:%02x:%02x",
             part[0], part[1], part[2], part[3], part[4], part[5]);

    return hex_addr;
  }

  /**
   * Get a string representation of this type
   *
   * @return A string representation of this type
   *
   * @note String representation format is in hex
   */
  std::string str() const
  { return hex_str(); }

  /**
   * Get a string representation of this type
   *
   * @return A string representation of this type
   *
   * @note String representation format is in hex
   */
  std::string to_string() const
  { return hex_str(); }

  /**
   * Operator to transform this type into string form
   *
   * @note String representation format is in hex
   */
  operator std::string () const
  { return hex_str(); }

  /**
   * Operator to check for equality
   *
   * @param other
   *  The MAC address object to check for equality
   *
   * @return true if this object is equal to other, false otherwise
   */
  constexpr bool operator==(const Addr other) const noexcept {
    return (minor == other.minor)
       and (major == other.major);
  }

  /**
   * Operator to check for inequality
   *
   * @param other
   *  The MAC address object to check for inequality
   *
   * @return true if this object is not equal to other, false otherwise
   */
  constexpr bool operator!=(const Addr other) const noexcept
  { return not (*this == other); }

  constexpr uint8_t operator[](uint8_t n) const noexcept
  {
      Expects(n < 6);
      return part[n];
  }

  /**
   * @brief      Construct a EUI (Extended Unique Identifier)
   *             from a 48-bit MAC addr
   *
   * @param[in]  addr  The address
   *
   * @return     A 64-bit EUI
   */
  static constexpr uint64_t eui64(const Addr& addr) noexcept
  {
    std::array<uint8_t, 8> eui {
      addr.part[0], addr.part[1], addr.part[2],
      0xFF, 0xFE,
      addr.part[3], addr.part[4], addr.part[5]
    };
    eui[0] ^= (1UL << 1);
    return *((uint64_t*)eui.data());
  }

  /**
   * @brief      Construct a EUI (Extended Unique Identifier)
   *             from this MAC addr
   *
   * @return     A 64-bit EUI
   */
  constexpr uint64_t eui64() const noexcept
  { return Addr::eui64(*this); }

  /**
   * @brief      Construct a broadcast/"multicast" MAC for
   *             the given IPv6 address.
   *
   * @param[in]  v6    The IPv6 address
   *
   * @tparam     IPv6  IPv6 address
   *
   * @return     A broadcast/"multicast" MAC address
   */
  template <typename IPv6>
  static Addr ipv6_mcast(const IPv6& v6)
  {
    return { 0x33, 0x33,
      v6.template get_part<uint8_t>(12),
      v6.template get_part<uint8_t>(13),
      v6.template get_part<uint8_t>(14),
      v6.template get_part<uint8_t>(15)};
  }


  static constexpr const size_t PARTS_LEN {6}; //< Number of parts in a MAC address
  uint8_t part[PARTS_LEN];                     //< The parts of the MAC address

  struct {
    uint16_t minor;
    uint32_t major;
  } __attribute__((packed));
} __attribute__((packed)); //< union Addr

constexpr const Addr EMPTY        {};
constexpr const Addr BROADCAST    {0xff,0xff,0xff,0xff,0xff,0xff}; //< uint16_t(0xFFFF), uint32_t(0xFFFFFFFF)
constexpr const Addr MULTICAST    {0x00,0x00,0x01,0x00,0x00,0x00}; //< uint16_t(0x0000), uint32_t(0x01000000)
constexpr const Addr IPv6mcast_01 {0x33,0x33,0x01,0x00,0x00,0x00}; //< uint16_t(0x3333), uint32_t(0x01000000)
constexpr const Addr IPv6mcast_02 {0x33,0x33,0x02,0x00,0x00,0x00}; //< uint16_t(0x3333), uint32_t(0x02000000)
} //< namespace MAC

#endif //< HW_MAC_ADDR_HPP
