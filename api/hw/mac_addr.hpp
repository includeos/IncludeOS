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
#pragma once

#ifndef HW_MAC_ADDR_HPP
#define HW_MAC_ADDR_HPP

#include <stddef.h>   // size_t
#include <stdint.h>   // uint
#include <cstdio>     // snprintf
#include <string>
#include <cstring>    // strncmp

namespace MAC {

  // MAC address
  union Addr {
    // no. parts in a MAC address
    static constexpr size_t PARTS_LEN = 6;
    // The parts of the MAC address
    uint8_t part[PARTS_LEN];

    struct {
      uint16_t minor;
      uint32_t major;
    } __attribute__((packed));

    Addr() noexcept : part{} {}

    Addr(const uint8_t a, const uint8_t b, const uint8_t c,
             const uint8_t d, const uint8_t e, const uint8_t f) noexcept
      : part{a,b,c,d,e,f}
    {}

    Addr& operator=(const Addr cpy) noexcept {
      minor = cpy.minor;
      major = cpy.major;
      return *this;
    }


    /**
     * @brief Hex representation of a MAC address
     *
     * @return hex string representation
     */
    std::string hex_str() const {
      char hex_addr[18];
      snprintf(hex_addr, sizeof(hex_addr), "%02x:%02x:%02x:%02x:%02x:%02x",
               part[0], part[1], part[2],
               part[3], part[4], part[5]);
      return hex_addr;
    }

    /**
     * @brief String representation of a MAC address
     * @note default is hex
     * @return string representation of the MAC address
     */
    std::string str() const
    { return hex_str(); }

    std::string to_string() const
    { return str(); }

    operator std::string () const
    { return str(); }

    /** Check for equality */
    bool operator==(const Addr mac) const noexcept
    {
      return strncmp(
                     reinterpret_cast<const char*>(part),
                     reinterpret_cast<const char*>(mac.part),
                     PARTS_LEN) == 0;
    }

    bool operator!=(const Addr mac) const noexcept
    { return not(*this == mac); }

  }  __attribute__((packed)); //< union addr

  const Addr EMPTY {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};

  // uint16_t(0xFFFF), uint32_t(0xFFFFFFFF)
  const Addr BROADCAST {0xff,0xff,0xff,0xff,0xff,0xff};

  // uint16_t(0x0000), uint32_t(0x01000000)
  const Addr MULTICAST {0,0,0x01,0,0,0};

  // uint16_t(0x3333), uint32_t(0x01000000)
  const Addr IPv6mcast_01 {0x33,0x33,0x01,0,0,0};

  // uint16_t(0x3333), uint32_t(0x02000000)
  const Addr IPv6mcast_02 {0x33,0x33,0x02,0,0,0};


}

#endif
