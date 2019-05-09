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

#pragma once

#include "common.hpp"
#include "header.hpp"
#include <net/ip4/packet_ip4.hpp>
#include <net/socket.hpp>
#include <cassert>

namespace net
{
  class PacketUDP : public PacketIP4
  {
  public:
    void init(uint16_t l_port, uint16_t d_port)
    {
      Expects(data_end() == layer_begin() + ip_header_length());

      // Initialize UDP packet header
      // source and destination ports
      set_src_port(l_port);
      set_dst_port(d_port);
      // set zero length
      set_length(sizeof(udp::Header));
      // zero the optional checksum
      header().checksum = 0;
      set_protocol(Protocol::UDP);
    }

    void set_src_port(uint16_t port) noexcept
    {
      header().sport = htons(port);
    }
    udp::port_t src_port() const noexcept
    {
      return htons(header().sport);
    }

    void set_dst_port(uint16_t port) noexcept
    {
      header().dport = htons(port);
    }
    udp::port_t dst_port() const noexcept
    {
      return htons(header().dport);
    }

    Socket source() const
    { return Socket{ip_src(), src_port()}; }

    Socket destination() const
    { return Socket{ip_dst(), dst_port()}; }

    uint16_t length() const noexcept
    {
      return ntohs(header().length);
    }

    void set_data_length(uint16_t len)
    {
      set_length(sizeof(udp::Header) + len);
    }

    uint16_t data_length() const noexcept
    {
      const uint16_t hdr_len = ip_header_length();
      uint16_t real_length = size() - hdr_len;
      uint16_t final_length = std::min(real_length, length());
      return final_length - sizeof(udp::Header);
    }

    Byte* data()
    {
      return ip_data_ptr() + sizeof(udp::Header);
    }

    const Byte* data() const
    { return ip_data_ptr() + sizeof(udp::Header); }

    Byte* begin()
    {
      return data();
    }

    Byte* begin_free()
    {
      return begin() + length();
    }

    uint16_t checksum()
    { return header().checksum; }

    void set_checksum(uint16_t check)
    { header().checksum = check; }

    // generates IP checksum for this packet
    // TODO: implement me
    uint16_t generate_checksum() const noexcept;

    //! assuming the packet has been properly initialized,
    //! this will fill bytes from @buffer into this packets buffer,
    //! then return the number of bytes written
    uint32_t fill(const std::string& buffer)
    {
      uint32_t rem   = buffer_end() - data_end();
      uint32_t total = (buffer.size() < rem) ? buffer.size() : rem;
      // copy from buffer to packet buffer
      memcpy(data_end(), buffer.data(), total);
      // set new packet length
      set_length(length() + total);
      return total;
    }

    bool validate_length() const noexcept {
      return length() >= sizeof(udp::Header);
    }

  private:
    // Sets the correct length for UDP and the packet
    void set_length(uint16_t newlen)
    {
      // new total UDP payload length
      header().length = htons(newlen);

      // new total packet length
      set_data_end(ip_header_length() + newlen);
    }

    udp::Header& header() noexcept
    {
      return *reinterpret_cast<udp::Header*>(ip_data_ptr());
    }

    const udp::Header& header() const noexcept
    {
      return *reinterpret_cast<const udp::Header*>(ip_data_ptr());
    }

  };
}
