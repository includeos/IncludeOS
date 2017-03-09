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

#ifndef IP4_PACKET_IP4_HPP
#define IP4_PACKET_IP4_HPP

#include <cassert>
#include <net/packet.hpp>
#include <net/inet_common.hpp>
#include <net/ip4/ip4.hpp>

namespace net {

  class PacketIP4 : public Packet
  {
  public:
    static constexpr size_t DEFAULT_TTL {64};

    const ip4::Addr& src() const noexcept
    { return ip_header().saddr; }

    void set_src(const ip4::Addr& addr) noexcept
    { ip_header().saddr = addr; }

    const ip4::Addr& dst() const noexcept
    { return ip_header().daddr; }

    void set_dst(const ip4::Addr& addr) noexcept
    { ip_header().daddr = addr; }

    void set_protocol(Protocol p) noexcept
    { ip_header().protocol = static_cast<uint8_t>(p); }

    Protocol protocol() const noexcept
    { return static_cast<Protocol>(ip_header().protocol); }

    uint16_t ip_segment_length() const noexcept
    { return ntohs(ip_header().tot_len); }

    uint8_t ip_header_length() const noexcept
    { return (ip_header().version_ihl & 0xf) * 4; }

    uint16_t ip_data_length() const noexcept
    { return ip_segment_length() - ip_header_length(); }

    uint16_t ip_capacity() const noexcept
    { return capacity() - ip_header_length(); }

    void set_ip_data_length(uint16_t length) {
      Expects(sizeof(IP4::header) + length <= (size_t) capacity());
      set_data_end(sizeof(IP4::header) + length);

      set_segment_length();
    }

    /** Last modifications before transmission */
    void make_flight_ready() noexcept {
      assert( ip_header().protocol );
      set_segment_length();
      set_ip4_checksum();
    }

    void init(Protocol proto = Protocol::HOPOPT) noexcept {
      Expects(size() == 0);
      auto& hdr = ip_header();
      hdr.version_ihl    = 0x45;
      hdr.tos            = 0;
      hdr.id             = 0;
      hdr.frag_off_flags = 0;
      hdr.ttl            = DEFAULT_TTL;
      hdr.protocol       = static_cast<uint8_t>(proto);
      set_ip_data_length(0);
    }

  protected:
    Byte* ip_data() noexcept __attribute__((assume_aligned(4)))
    {
      return layer_begin() + ip_header_length();
    }

    const Byte* ip_data() const noexcept __attribute__((assume_aligned(4)))
    {
      return layer_begin() + ip_header_length();
    }

    /**
     *  Set IP4 header length
     *  Inferred from packet size
     */
    void set_segment_length() noexcept
    { ip_header().tot_len = htons(size()); }

  private:

    void set_header_length() noexcept
    { }

    const ip4::Header& ip_header() const noexcept
    { return *reinterpret_cast<const IP4::header*>(layer_begin()); }

    ip4::Header& ip_header() noexcept
    { return *reinterpret_cast<IP4::header*>(layer_begin()); }

    void set_ip4_checksum() noexcept {
      auto& hdr = ip_header();
      hdr.check = 0;
      hdr.check = net::checksum(&hdr, ip_header_length());
    }

  }; //< class PacketIP4
} //< namespace net

#endif //< IP4_PACKET_IP4_HPP
