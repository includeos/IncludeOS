// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#include <cstdint>

namespace net {

  /* RFC 6335 - IANA */
  namespace port_ranges
  {
    static constexpr uint16_t SYSTEM_START  {0};
    static constexpr uint16_t SYSTEM_END    {1023};
    static constexpr uint16_t USER_START    {1024};
    static constexpr uint16_t USER_END      {49151};
    static constexpr uint16_t DYNAMIC_START {49152};
    static constexpr uint16_t DYNAMIC_END   {65535};

    static constexpr bool is_dynamic(const uint16_t port) noexcept
    { return port >= DYNAMIC_START; }
  }

  /**
   * Known transport layer protocols. Shared between IPv4 and IPv6.
   * Full list:
   * http://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
   */
  enum class Protocol : uint8_t {
    HOPOPT     =  0, // Hop-by-Hop Options Header
    ICMPv4     =  1,
    IPv4       =  4, // IPv4 encapsulation
    TCP        =  6,
    UDP        = 17,
    IPv6       = 41, // IPv6 encapsulation
    IPv6_ROUTE = 43, // Routing Header
    IPv6_FRAG  = 44, // Fragment Header
    RSVP       = 46, // Resource ReSerVation Protocol
    ESP        = 50, // Encapsulating Security Payload
    AH         = 51, // Authentication Header
    ICMPv6     = 58,
    IPv6_NONXT = 59, // No next header
    OPTSV6     = 60  // Destination Options Header
  };

  /**
   * Explicit Congestion Notification (ECN) values for IPv4 and IPv6
   * Defined in RFC3168
   **/
  enum class ECN : uint8_t {
    NOT_ECT = 0b00,   // Non-ECN transport
    ECT_0   = 0b01,   // ECN-enabled transport 1
    ECT_1   = 0b10,   // ECN-enabled transport 2
    CE      = 0b11    // Congestion encountered
  };

  /**
   * Differentiated Services Code Points, for IPv4 and IPv6
   * Defined in RFC2474
   * NOTE: Replaces IPv4 TOS field
   *
   * IANA list:
   * https://www.iana.org/assignments/dscp-registry/dscp-registry.xhtml
   *
   * 6 DSCP bits together with 2 ECN bits form one octet
   **/
  enum class DSCP : uint8_t {
    CS0 = 0b000000,
    CS1 = 0b001000,
    CS2 = 0b010000,
    CS3 = 0b011000,
    CS4 = 0b100000,
    CS5 = 0b101000,
    CS6 = 0b110000,
    CS7 = 0b111000,
    AF11 = 0b001010,
    AF12 = 0b001100,
    AF13 = 0b001110,
    AF21 = 0b010010,
    AF22 = 0b010100,
    AF23 = 0b010110,
    AF31 = 0b011010,
    AF32 = 0b011100,
    AF33 = 0b011110,
    AF41 = 0b100010,
    AF42 = 0b100100,
    AF43 = 0b100110,
    EF_PHB = 0b101110,
    VOICE_ADMIT = 0b101100
  };

}
