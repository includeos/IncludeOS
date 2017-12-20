// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

/** Common utilities for internetworking */

#ifndef NET_INET_COMMON_HPP
#define NET_INET_COMMON_HPP

#include <delegate>
#include <net/packet.hpp>
#include <hw/mac_addr.hpp>
#include <net/ethernet/ethertype.hpp>
#include <net/ip4/icmp4_common.hpp>
#include <net/socket.hpp>
#include <net/checksum.hpp>

#include <net/error.hpp>
#include <net/ip4/icmp_error.hpp>

namespace net {
  // Packet must be forward declared to avoid circular dependency
  // i.e. IP uses Packet, and Packet uses IP headers
  class Packet;
  class Ethernet;

  using LinkLayer = Ethernet;

  using Packet_ptr = std::unique_ptr<Packet>;
  using Frame_ptr = std::unique_ptr<Packet>;

  // Downstream / upstream delegates
  using downstream = delegate<void(Packet_ptr)>;
  using downstream_link = delegate<void(Packet_ptr, MAC::Addr, Ethertype)>;
  using upstream = downstream;
  using upstream_ip = delegate<void(Packet_ptr, const bool link_bcast)>;

  // Delegate for signalling available buffers in device transmit queue
  using transmit_avail_delg = delegate<void(size_t)>;

  // View a packet differently based on context
  template <typename T, typename Packet>
  inline auto view_packet_as(Packet packet) noexcept {
    return std::static_pointer_cast<T>(packet);
  }

  template<typename Derived, typename Base>
  auto static_unique_ptr_cast( std::unique_ptr<Base>&& p )
  {
      auto* d = static_cast<Derived *>(p.release());
      return std::unique_ptr<Derived>(d);
  }

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
    HOPOPT  =  0,
    ICMPv4  =  1,
    IPv4    =  4,  // IPv4 encapsulation
    TCP     =  6,
    UDP     = 17,
    IPv6    = 41,  // IPv6 encapsulation
    ICMPv6  = 58
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

  inline uint16_t new_ephemeral_port() noexcept
  { return port_ranges::DYNAMIC_START + rand() % (port_ranges::DYNAMIC_END - port_ranges::DYNAMIC_START); }

} //< namespace net

#endif //< NET_INET_COMMON_HPP
