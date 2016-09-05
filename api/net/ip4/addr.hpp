// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#ifndef NET_IP4_ADDR_HPP
#define NET_IP4_ADDR_HPP

#include <regex>
#include <string>
#include <net/util.hpp> // byte order

namespace net {
namespace ip4 {

  class Invalid_address : public std::runtime_error {
    using runtime_error::runtime_error;
  };

/** IP4 address representation */
struct Addr {

  Addr() : whole(0) {} // uninitialized
  Addr(const uint32_t ipv4_addr)
    : whole(ipv4_addr) {}
  Addr(const uint8_t p1, const uint8_t p2, const uint8_t p3, const uint8_t p4)
    : whole(p1 | (p2 << 8) | (p3 << 16) | (p4 << 24)) {}

  /**
   * @brief Construct an IPv4 address from a {std::string}
   * object
   *
   * @note If the {std::string} object doesn't contain a valid
   * IPv4 representation then the instance will contain the
   * address -> 0.0.0.0
   *
   * @param ipv4_addr:
   * A {std::string} object representing an IPv4 address
   */
  Addr(const std::string& ipv4_addr)
    : Addr{}
  {
    if (not(ipv4_addr.size() >= 7 && ipv4_addr.size() <= 15)) //< [7, 15] minimum and maximum address length
      throw Invalid_address(ipv4_addr + " is not a valid IP");
    const static std::regex ipv4_address_pattern
    {
      "^\\s*(25[0–5]|2[0–4]\\d|[01]?\\d\\d?)\\."
        "(25[0–5]|2[0–4]\\d|[01]?\\d\\d?)\\."
        "(25[0–5]|2[0–4]\\d|[01]?\\d\\d?)\\."
        "(25[0–5]|2[0–4]\\d|[01]?\\d\\d?)\\s*$"
    };

    std::smatch ipv4_parts;

    if (not std::regex_match(ipv4_addr, ipv4_parts, ipv4_address_pattern)) {
      throw Invalid_address(ipv4_addr + " is not a valid IP");
    }

    const auto p1 = static_cast<uint8_t>(std::stoi(ipv4_parts[1]));
    const auto p2 = static_cast<uint8_t>(std::stoi(ipv4_parts[2]));
    const auto p3 = static_cast<uint8_t>(std::stoi(ipv4_parts[3]));
    const auto p4 = static_cast<uint8_t>(std::stoi(ipv4_parts[4]));

    whole = p1 | (p2 << 8) | (p3 << 16) | (p4 << 24);
  }

  inline Addr& operator=(const Addr cpy) noexcept {
    whole = cpy.whole;
    return *this;
  }

  /** Standard comparison operators */
  bool operator==(const Addr rhs)     const noexcept
  { return (*this == rhs.whole); }

  bool operator==(const uint32_t rhs) const noexcept
  { return whole == rhs; }

  bool operator<(const Addr rhs)      const noexcept
  { return (*this < rhs.whole); }

  bool operator<(const uint32_t rhs)  const noexcept
  { return  ntohl(whole) < ntohl(rhs); }

  bool operator>(const Addr rhs)      const noexcept
  { return !(*this < rhs); }

  bool operator>(const uint32_t rhs)  const noexcept
  { return !(*this < rhs); }

  bool operator!=(const Addr rhs)     const noexcept
  { return !(*this == rhs); }

  bool operator!=(const uint32_t rhs) const noexcept
  { return !(*this == rhs); }

  Addr operator & (const Addr rhs)    const noexcept
  { return Addr(whole & rhs.whole); }

  Addr operator | (const Addr rhs)    const noexcept
  { return Addr(whole | rhs.whole); }

  Addr operator ~ () const noexcept
  { return Addr(~whole); }


  /** x.x.x.x string representation */
  std::string str() const {
    char ipv4_addr[16];
    snprintf(ipv4_addr, sizeof(ipv4_addr),
            "%1i.%1i.%1i.%1i",
            (whole >>  0) & 0xFF,
            (whole >>  8) & 0xFF,
            (whole >> 16) & 0xFF,
            (whole >> 24) & 0xFF);
    return ipv4_addr;
  }

  std::string to_string() const
  { return str(); }

  uint32_t whole;

} __attribute__((packed)); //< Addr

} // < namespace ip4
} // < namespace net


#endif // < NET_IP4_ADDR_HPP
