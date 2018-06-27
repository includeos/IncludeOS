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
#include <net/ip4/packet_ip4.hpp>

namespace net::tcp {

template <typename Ptr_type> class Packet4_v;
using Packet4_view = Packet4_v<net::Packet_ptr>;
using Packet4_view_raw = Packet4_v<net::Packet*>;

template <typename Ptr_type>
class Packet4_v : public Packet_v<Ptr_type> {
public:
  Packet4_v(Ptr_type ptr)
    : Packet_v<Ptr_type>(std::move(ptr))
  {
    Expects(packet().is_ipv4());
    this->set_header(packet().ip_data().data());
  }

  inline void init();

  uint16_t compute_tcp_checksum() const noexcept override
  { return calculate_checksum4(*this); }

  Protocol ipv() const noexcept override
  { return Protocol::IPv4; }

  ip4::Addr ip4_src() const noexcept
  { return packet().ip_src(); }

  ip4::Addr ip4_dst() const noexcept
  { return packet().ip_dst(); }

private:
  PacketIP4& packet() noexcept
  { return static_cast<PacketIP4&>(*this->pkt); }

  const PacketIP4& packet() const noexcept
  { return static_cast<PacketIP4&>(*this->pkt); }

  void set_ip_src(const net::Addr& addr) noexcept override
  { packet().set_ip_src(addr.v4()); }

  void set_ip_dst(const net::Addr& addr) noexcept override
  { packet().set_ip_dst(addr.v4()); }

  net::Addr ip_src() const noexcept override
  { return packet().ip_src(); }

  net::Addr ip_dst() const noexcept override
  { return packet().ip_dst(); }

  uint16_t ip_data_length() const noexcept override
  { return packet().ip_data_length(); }

  uint16_t ip_header_length() const noexcept override
  { return packet().ip_header_length(); }

};

template <typename Ptr_type>
inline void Packet4_v<Ptr_type>::init()
{
  // clear TCP header
  memset(this->header, 0, sizeof(tcp::Header));

  this->set_win(tcp::default_window_size);
  this->header->offset_flags.offset_reserved = (5 << 4);

  this->set_length();
}

}
