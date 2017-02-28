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

  // Delegate for signalling available buffers in device transmit queue
  using transmit_avail_delg = delegate<void(size_t)>;

  // Compute internet checksum with partial @sum provided
  uint16_t checksum(uint32_t sum, const void* data, size_t len) noexcept;

  // Compute the internet checksum for the buffer / buffer part provided
  inline uint16_t checksum(const void* data, size_t len) noexcept {
    return checksum(0, data, len);
  }

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
    static constexpr uint16_t DYNAMIC_END   {65534}; // 65535 never assigned
  }

  /**
   * Known transport layer protocols. Shared between IPv4 and IPv6.
   * Full list:
   * http://www.iana.org/assignments/protocol-numbers/protocol-numbers.xhtml
   */
  enum class Protocol : uint8_t {
    HOPOPT = 0,
      ICMPv4  =  1,
      IP4v4   =  4,  // IPv4 encapsulation
      TCP     =  6,
      UDP     = 17,
      IPv6    = 41,  // IPv6 encapsulation
      ICMPv6  = 58
      };



  inline uint16_t new_ephemeral_port() noexcept
  { return port_ranges::DYNAMIC_START + rand() % (port_ranges::DYNAMIC_END - port_ranges::DYNAMIC_START); }

} //< namespace net

#endif //< NET_INET_COMMON_HPP
