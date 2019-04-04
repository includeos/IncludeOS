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

#include "common.hpp"
#include "header.hpp"

#include <net/socket.hpp>
#include <net/packet.hpp>
#include <net/iana.hpp>

namespace net::udp {

template <typename Ptr_type> class Packet_v;
using Packet_view = Packet_v<net::Packet_ptr>;
using Packet_view_raw = Packet_v<net::Packet*>;
using Packet_view_ptr = std::unique_ptr<Packet_view>;

template <typename Ptr_type>
class Packet_v {
public:

  const Header& udp_header() const noexcept
  { return *header; }

  // Ports etc //

  port_t src_port() const noexcept
  { return ntohs(udp_header().sport); }

  port_t dst_port() const noexcept
  { return ntohs(udp_header().dport); }

  net::Socket source() const noexcept
  { return {ip_src(), src_port()}; }

  net::Socket destination() const noexcept
  { return {ip_dst(), dst_port()}; }

  Packet_v& set_src_port(port_t p) noexcept
  { udp_header().sport = htons(p); return *this; }

  Packet_v& set_dst_port(port_t p) noexcept
  { udp_header().dport = htons(p); return *this; }

  Packet_v& set_source(const net::Socket& src)
  {
    set_ip_src(src.address());
    set_src_port(src.port());
    return *this;
  }

  Packet_v& set_destination(const net::Socket& dest)
  {
    set_ip_dst(dest.address());
    set_dst_port(dest.port());
    return *this;
  }

  virtual void set_ip_src(const net::Addr& addr) noexcept = 0;
  virtual void set_ip_dst(const net::Addr& addr) noexcept = 0;
  virtual net::Addr ip_src() const noexcept = 0;
  virtual net::Addr ip_dst() const noexcept = 0;

  constexpr uint16_t udp_header_length() const noexcept
  { return sizeof(udp::Header); }

  uint16_t udp_length() const noexcept
  { return udp_header_length() + udp_data_length(); }

  uint16_t udp_data_length() const noexcept
  {
    const uint16_t hdr_len = ip_header_length();
    uint16_t real_length = pkt->size() - hdr_len;
    uint16_t final_length = std::min(real_length, ntohs(udp_header().length));
    return final_length - sizeof(udp::Header);
  }

  bool validate_length() const noexcept
  { return udp_length() >= sizeof(udp::Header); }

  uint16_t udp_checksum() const noexcept
  { return udp_header().checksum; }

  virtual uint16_t compute_udp_checksum() const noexcept
  { return 0x0; }

  void set_udp_checksum() noexcept
  {
    udp_header().checksum = 0;
    udp_header().checksum = compute_udp_checksum();
  }

  uint8_t* udp_data()
  { return (uint8_t*)header + udp_header_length(); }

  const uint8_t* udp_data() const
  { return (uint8_t*)header + udp_header_length(); }

  inline size_t fill(const uint8_t* buffer, size_t length);

  // Packet_view specific operations //

  Ptr_type release()
  {
    Expects(pkt != nullptr && "Packet ptr is already null");
    return std::move(pkt);
  }

  const Ptr_type& packet_ptr() const noexcept
  { return pkt; }

  // hmm
  virtual Protocol ipv() const noexcept = 0;

  virtual ~Packet_v() = default;


protected:
  Ptr_type   pkt;
  Header*    header = nullptr;

  Packet_v(Ptr_type ptr)
    : pkt{std::move(ptr)}
  {
    Expects(pkt != nullptr);
  }

  Header& udp_header() noexcept
  { return *header; }

  void set_header(uint8_t* hdr)
  { Expects(hdr != nullptr); header = reinterpret_cast<Header*>(hdr); }

  // sets the correct length for all the protocols up to IP4
  void set_length(uint16_t newlen = 0)
  {
    // new total UDP payload length
    udp_header().length = htons(udp_header_length() + newlen);
    pkt->set_data_end(ip_header_length() + udp_header_length() + newlen);
  }


private:
  // TODO: see if we can get rid of these virtual calls
  virtual uint16_t ip_data_length() const noexcept = 0;
  virtual uint16_t ip_header_length() const noexcept = 0;
  uint16_t ip_capacity() const noexcept
  { return pkt->capacity() - ip_header_length(); }

};

template <typename Ptr_type>
inline size_t Packet_v<Ptr_type>::fill(const uint8_t* buffer, size_t length)
{
  size_t rem = ip_capacity() - udp_length();
  if(rem == 0) return 0;
  size_t total = std::min(length, rem);
  // copy from buffer to packet buffer
  memcpy(udp_data() + udp_data_length(), buffer, total);
  // set new packet length
  set_length(udp_data_length() + total);
  return total;
}


}
