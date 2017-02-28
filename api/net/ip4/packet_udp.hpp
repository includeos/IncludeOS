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

#include "udp.hpp"
#include "packet_ip4.hpp"
#include <cassert>

namespace net
{
  class PacketUDP : public PacketIP4
  {
  public:

    UDP::header& header() noexcept
    {
      return *reinterpret_cast<UDP::header*>(ip_data());
    }

    const UDP::header& header() const noexcept
    {
      return *reinterpret_cast<const UDP::header*>(ip_data());
    }

    //! initializes to a default, empty UDP packet, given
    //! a valid MTU-sized buffer
    void init()
    {

      // Promote to IP4 packet
      // PacketIP4::init();
      Expects(layer_begin() > buf());
      Expects(data_end() == layer_begin() + ip_header_length());

      // Promote to UDP packet
      increment_data_end(sizeof(UDP::header));
      Ensures(data_end() == layer_begin() + ip_header_length() + sizeof(UDP::header));

      // Initialize UDP packet header
      // source and destination ports
      header().sport = 0;
      header().dport = 0;
      // set zero length
      set_length(0);
      // zero the optional checksum
      header().checksum = 0;
      set_protocol(Protocol::UDP);
    }

    UDP::port_t src_port() const
    {
      return htons(header().sport);
    }

    UDP::port_t dst_port() const
    {
      return htons(header().dport);
    }

    uint16_t length() const
    {
      return ntohs(header().length);
    }

    uint16_t data_length() const
    {
      return length() - sizeof(UDP::header);
    }

    inline Byte* data()
    {
      return ip_data() + sizeof(UDP::header);
    }

    inline Byte* begin()
    {
      return data();
    }

    inline Byte* begin_free()
    {
      return begin() + length();
    }

    // Sets the correct length for UDP and the packet
    void set_length(uint16_t newlen)
    {
      // new total UDP payload length
      header().length = htons(sizeof(UDP::header) + newlen);

      // new total packet length
      increment_data_end( sizeof(UDP::header) + newlen );
    }

    // generates a new checksum and sets it for this UDP packet
    uint16_t gen_checksum();

    //! assuming the packet has been properly initialized,
    //! this will fill bytes from @buffer into this packets buffer,
    //! then return the number of bytes written. buffer is unmodified
    uint32_t fill(const std::string& buffer)
    {
      uint32_t rem = capacity();
      uint32_t total = (buffer.size() < rem) ? buffer.size() : rem;
      // copy from buffer to packet buffer
      memcpy(data() + data_length(), buffer.data(), total);
      // set new packet length
      set_length(data_length() + total);
      return total;
    }
  };
}
