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
#ifndef NET_TCP_COMMON_HPP
#define NET_TCP_COMMON_HPP

#include <net/addr.hpp>
#include <net/checksum.hpp>
#include <chrono>

#include <util/units.hpp>
#include <vector>
#include <pmr>

namespace net {
  namespace tcp {

    // Constants
    // default size of TCP window - how much data can be "in flight" (unacknowledged)
    static constexpr uint16_t default_window_size {0xffff};
    // window scaling + window size
    static constexpr uint8_t  default_window_scaling {5};
    static constexpr uint32_t default_ws_window_size {8192 << default_window_scaling};
    // use of timestamps option
    static constexpr bool     default_timestamps {true};
    // use of SACK
    static constexpr bool     default_sack {true};
    static constexpr size_t   default_sack_entries{32};
    // maximum size of a TCP segment - later set based on MTU or peer
    static constexpr uint16_t default_mss     {536};
    static constexpr uint16_t default_mss_v6  {1220};
    // the maximum amount of half-open connections per port (listener)
    static constexpr size_t   default_max_syn_backlog {64};
    // clock granularity of the timestamp value clock
    static constexpr float   clock_granularity {0.0001};

    static const std::chrono::seconds       default_msl {30};
    static const std::chrono::milliseconds  default_dack_timeout {40};

    using namespace util::literals;
    static constexpr size_t default_min_bufsize   {4_KiB};
    static constexpr size_t default_max_bufsize   {256_KiB};
    static constexpr size_t default_total_bufsize {64_MiB};

    using Address = net::Addr;

    /** A port */
    using port_t = uint16_t;

    /** A Sequence number (SYN/ACK) (32 bits) */
    using seq_t = uint32_t;

    /** A shared buffer pointer */
    using buffer_t = os::mem::buf_ptr;

    /** Construct a shared vector used in TCP **/
    template <typename... Args>
    buffer_t construct_buffer(Args&&... args) {
      return std::make_shared<os::mem::buffer> (std::forward<Args> (args)...);
    }

    class Connection;
    using Connection_ptr = std::shared_ptr<Connection>;

    // TODO: Remove when TCP packet class is gone
    template <typename Tcp_packet>
    uint16_t calculate_checksum(const Tcp_packet& packet)
    {
      constexpr uint8_t Proto_TCP = 6; // avoid including inet_common
      uint16_t length = packet.tcp_length();
      // Compute sum of pseudo-header
      uint32_t sum =
            (packet.ip_src().whole >> 16)
          + (packet.ip_src().whole & 0xffff)
          + (packet.ip_dst().whole >> 16)
          + (packet.ip_dst().whole & 0xffff)
          + (Proto_TCP << 8)
          + htons(length);

      // Compute sum of header and data
      const char* buffer = (char*) &packet.tcp_header();
      return net::checksum(sum, buffer, length);
    }

    template <typename View4>
    uint16_t calculate_checksum4(const View4& packet)
    {
      constexpr uint8_t Proto_TCP = 6; // avoid including inet_common
      uint16_t length = packet.tcp_length();
      const auto ip_src = packet.ip4_src();
      const auto ip_dst = packet.ip4_dst();
      // Compute sum of pseudo-header
      uint32_t sum =
            (ip_src.whole >> 16)
          + (ip_src.whole & 0xffff)
          + (ip_dst.whole >> 16)
          + (ip_dst.whole & 0xffff)
          + (Proto_TCP << 8)
          + htons(length);

      // Compute sum of header and data
      const char* buffer = (char*) &packet.tcp_header();
      return net::checksum(sum, buffer, length);
    }

    template <typename View6>
    uint16_t calculate_checksum6(const View6& packet)
    {
      constexpr uint8_t Proto_TCP = 6; // avoid including inet_common
      uint16_t length = packet.tcp_length();
      const auto ip_src = packet.ip6_src();
      const auto ip_dst = packet.ip6_dst();
      // Compute sum of pseudo-header
      uint32_t sum = 0;

      for(int i = 0; i < 4; i++)
      {
        uint32_t part = ip_src.template get_part<uint32_t>(i);
        sum += (part >> 16);
        sum += (part & 0xffff);

        part = ip_dst.template get_part<uint32_t>(i);
        sum += (part >> 16);
        sum += (part & 0xffff);
      }

      sum += (Proto_TCP << 8) + htons(length);

      // Compute sum of header and data
      const char* buffer = (char*) &packet.tcp_header();
      return net::checksum(sum, buffer, length);
    }

  } // < namespace tcp
} // < namespace net

#endif // < NET_TCP_COMMON_HPP
