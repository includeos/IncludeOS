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

#include "packet_view.hpp"
#include <net/ip6/packet_ip6.hpp>

namespace net::udp {

template <typename Ptr_type> class Packet6_v;
using Packet6_view = Packet6_v<net::Packet_ptr>;
using Packet6_view_raw = Packet6_v<net::Packet*>;

template <typename Ptr_type>
class Packet6_v : public Packet_v<Ptr_type> {
public:
  Packet6_v(Ptr_type ptr)
    : Packet_v<Ptr_type>(std::move(ptr))
  {
    Expects(packet().is_ipv6());
    this->set_header(packet().ip_data().data());
  }

  inline void init(const net::Socket& src, const net::Socket& dst);

  Protocol ipv() const noexcept override
  { return Protocol::IPv6; }

  ip6::Addr ip6_src() const noexcept
  { return packet().ip_src(); }

  ip6::Addr ip6_dst() const noexcept
  { return packet().ip_dst(); }

  uint16_t compute_udp_checksum() const noexcept override
  { return calculate_checksum6(*this); }

private:
  PacketIP6& packet() noexcept
  { return static_cast<PacketIP6&>(*this->pkt); }

  const PacketIP6& packet() const noexcept
  { return static_cast<PacketIP6&>(*this->pkt); }

  void set_ip_src(const net::Addr& addr) noexcept override
  { packet().set_ip_src(addr.v6()); }

  void set_ip_dst(const net::Addr& addr) noexcept override
  { packet().set_ip_dst(addr.v6()); }

  net::Addr ip_src() const noexcept override
  { return packet().ip_src(); }

  net::Addr ip_dst() const noexcept override
  { return packet().ip_dst(); }

  uint16_t ip_data_length() const noexcept override
  { return packet().ip_data_length(); }

  uint16_t ip_header_length() const noexcept override
  { return packet().ip_header_len(); }
};

template <typename Ptr_type>
inline void Packet6_v<Ptr_type>::init(const net::Socket& src, const net::Socket& dst)
{
  Expects(src.address().is_v6() and dst.address().is_v6());
  // set zero length
  this->set_length();
  // zero the optional checksum
  this->udp_header().checksum = 0;

  this->set_source(src);
  this->set_destination(dst);
}

}
