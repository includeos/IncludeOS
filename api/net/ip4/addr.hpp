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
#ifndef NET_IP4_ADDR_HPP
#define NET_IP4_ADDR_HPP

#include <regex>
#include <string>

#include <common>
#include <net/util.hpp>

namespace net {
namespace ip4 {

/**
 * This type is thrown when creating an instance of Addr
 * with a std::string that doesn't represent a valid IPv4
 * address
 */
struct Invalid_address : public std::runtime_error {
  using runtime_error::runtime_error;
}; //< struct Invalid_address

/**
 * IPv4 address representation
 */
struct Addr {
  /**
   * Constructor
   *
   * Create an IPv4 address object to represent the address <0.0.0.0>
   */
  Addr() noexcept
    : whole{}
  {}

  /**
   * Constructor
   *
   * Create an IPv4 address using a 32-bit value
   *
   * @param ipv4_addr
   *  The 32-bit value representing the IPv4 address
   *
   * @note The 32-bit value must be in network byte order
   */
  Addr(const uint32_t ipv4_addr) noexcept
    : whole{ipv4_addr}
  {}

  /**
   * Constructor
   *
   * Create an IPv4 address by specifying each part of the address
   *
   * @param p1
   *  The first part of the IPv4 address
   *
   * @param p2
   *  The second part of the IPv4 address
   *
   * @param p3
   *  The third part of the IPv4 address
   *
   * @param p4
   *  The fourth part of the IPv4 address
   */
  Addr(const uint8_t p1, const uint8_t p2, const uint8_t p3, const uint8_t p4) noexcept
    : whole(p1 | (p2 << 8) | (p3 << 16) | (p4 << 24))
  {}

  /**
   * Constructor
   *
   * Construct an IPv4 address from a {std::string} object
   * representing an IPv4 address
   *
   * @param ipv4_addr
   * A {std::string} object representing an IPv4 address
   *
   * @throws Invalid_address
   *  IIf the {std::string} object doesn't representing a valid IPv4
   *  address
   */
  template<typename = void>
  Addr(const std::string& ipv4_addr)
  {
    if (not (ipv4_addr.size() >= 7 && ipv4_addr.size() <= 15)) { //< [7, 15] minimum and maximum address length
      throw Invalid_address{ipv4_addr + " is not a valid IP"};
    }

    const static std::regex ipv4_address_pattern
    {
      "^\\s*(25[0–5]|2[0–4]\\d|[01]?\\d\\d?)\\."
        "(25[0–5]|2[0–4]\\d|[01]?\\d\\d?)\\."
        "(25[0–5]|2[0–4]\\d|[01]?\\d\\d?)\\."
        "(25[0–5]|2[0–4]\\d|[01]?\\d\\d?)\\s*$"
    };

    std::smatch ipv4_parts;

    if (not std::regex_match(ipv4_addr, ipv4_parts, ipv4_address_pattern)) {
      throw Invalid_address{ipv4_addr + " is not a valid IP"};
    }

    const auto p1 = static_cast<uint8_t>(std::stoi(ipv4_parts[1]));
    const auto p2 = static_cast<uint8_t>(std::stoi(ipv4_parts[2]));
    const auto p3 = static_cast<uint8_t>(std::stoi(ipv4_parts[3]));
    const auto p4 = static_cast<uint8_t>(std::stoi(ipv4_parts[4]));

    whole = p1 | (p2 << 8) | (p3 << 16) | (p4 << 24);
  }

  /**
   * Assignment operator
   *
   * @param other
   *  The IPv4 address object to assign from
   *
   * @return The object that invoked this method
   */
  Addr& operator=(const Addr other) noexcept {
    whole = other.whole;
    return *this;
  }

  /**
   * Operator to check for equality
   *
   * @param other
   *  The IPv4 address object to check for equality
   *
   * @return true if this object is equal to other, false otherwise
   */
  bool operator==(const Addr other) const noexcept
  { return whole == other.whole; }

  /**
   * Operator to check for equality
   *
   * @param raw_addr
   *  The 32-bit value to check for equality
   *
   * @return true if this object is equal to raw_addr, false otherwise
   *
   * @note The 32-bit value must be in network byte order
   */
  bool operator==(const uint32_t raw_addr) const noexcept
  { return whole == raw_addr; }

  /**
   * Operator to check for inequality
   *
   * @param other
   *  The IPv4 address object to check for inequality
   *
   * @return true if this object is not equal to other, false otherwise
   */
  bool operator!=(const Addr other) const noexcept
  { return not (*this == other); }

  /**
   * Operator to check for inequality
   *
   * @param raw_addr
   *  The 32-bit value to check for inequality
   *
   * @return true if this object is equal to not raw_addr, false otherwise
   *
   * @note The 32-bit value must be in network byte order
   */
  bool operator!=(const uint32_t raw_addr) const noexcept
  { return not (*this == raw_addr); }

  /**
   * Operator to check for less-than relationship
   *
   * @param other
   *  The IPv4 address object to check for less-than relationship
   *
   * @return true if this object is less-than other, false otherwise
   */
  bool operator<(const Addr other) const noexcept
  { return ntohl(whole) < ntohl(other.whole); }

  /**
   * Operator to check for less-than relationship
   *
   * @param raw_addr
   *  The 32-bit value to check for less-than relationship
   *
   * @return true if this object is less-than raw_addr, false otherwise
   *
   * @note The 32-bit value must be in network byte order
   */
  bool operator<(const uint32_t raw_addr) const noexcept
  { return ntohl(whole) < ntohl(raw_addr); }

  /**
   * Operator to check for greater-than relationship
   *
   * @param other
   *  The IPv4 address object to check for greater-than relationship
   *
   * @return true if this object is greater-than other, false otherwise
   */
  bool operator>(const Addr other) const noexcept
  { return not (*this < other); }

  /**
   * Operator to check for greater-than relationship
   *
   * @param raw_addr
   *  The 32-bit value to check for greater-than relationship
   *
   * @return true if this object is greater-than raw_addr, false otherwise
   *
   * @note The 32-bit value must be in network byte order
   */
  bool operator>(const uint32_t raw_addr) const noexcept
  { return not (*this < raw_addr); }

  /**
   * Operator to perform a bitwise-and operation on the given
   * IPv4 addresses
   *
   * @param other
   *  The IPv4 address object to perform the bitwise-and operation
   *
   * @return An IPv4 address object containing the result of the
   * operation
   */
  Addr operator&(const Addr other) const noexcept
  { return Addr{whole & other.whole}; }

  /**
   * Operator to perform a bitwise-or operation on the given
   * IPv4 addresses
   *
   * @param other
   *  The IPv4 address object to perform the bitwise-or operation
   *
   * @return An IPv4 address object containing the result of the
   * operation
   */
  Addr operator|(const Addr other) const noexcept
  { return Addr{whole | other.whole}; }

  /**
   * Operator to perform a bitwise-not operation on the IPv4
   * address
   *
   * @return An IPv4 address object containing the result of the
   * operation
   */
  Addr operator~() const noexcept
  { return Addr{~whole}; }

  /**
   * Get a part from the IPv4 address
   *
   * @param n
   *  The part number
   *
   * @return A part from the IPv4 address
   */
  uint8_t part(const uint8_t n) const {
    Expects(n < 4);

    const union addr_t {
      uint32_t whole;
      uint8_t  parts[4];
    } cubicles {this->whole};

    return cubicles.parts[3 - n];
  }

  /**
   * Get a string representation of the IPv4 address
   *
   * @return A string representation of the IPv4 address
   */
  std::string str() const {
    char ipv4_addr[16];

    snprintf(ipv4_addr, sizeof(ipv4_addr), "%1i.%1i.%1i.%1i",
            (whole >>  0) & 0xFF,
            (whole >>  8) & 0xFF,
            (whole >> 16) & 0xFF,
            (whole >> 24) & 0xFF);

    return ipv4_addr;
  }

  /**
   * Get a string representation of this type
   *
   * @return A string representation of this type
   */
  std::string to_string() const
  { return str(); }

  /* Data member */
  uint32_t whole;
} __attribute__((packed)); //< struct Addr

} //< namespace ip4
} //< namespace net

// Allow an IPv4 address to be used as key in e.g. std::unordered_map
namespace std {
  template<>
  struct hash<net::ip4::Addr> {
    size_t operator()(const net::ip4::Addr& addr) const {
      return std::hash<uint32_t>{}(addr.whole);
    }
  };
} //< namespace std

#endif //< NET_IP4_ADDR_HPP
