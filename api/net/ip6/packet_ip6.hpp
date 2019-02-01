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
#ifndef IP6_PACKET_IP6_HPP
#define IP6_PACKET_IP6_HPP

#include "header.hpp"
#include "extension_header.hpp"
#include <cassert>
#include <net/packet.hpp>

namespace net
{
  enum {
      OPT_HOP,
      OPT_V6,
      OPT_MAX
  };

  /** IPv6 packet. */
  class PacketIP6 : public Packet {
  public:
    static constexpr int DEFAULT_HOP_LIMIT = 64;
    using Span = gsl::span<Byte>;
    using Cspan = gsl::span<const Byte>;

    // IPv6 header getters

    uint32_t ver_tc_fl() const noexcept
    { return ip6_header().ver_tc_fl; }

    uint8_t ip6_version() const noexcept
    { return (ver_tc_fl() & 0xF0) >> 4; }

    bool is_ipv6() const noexcept
    { return ip6_version() == 6; }

    /** Get traffic class */
    uint8_t traffic_class() const noexcept
    { return ((ver_tc_fl() & 0xF0000) >> 12) +
        (ver_tc_fl() & 0xF); }

    /** Get Differentiated Services Code Point (DSCP)*/
    DSCP ip_dscp() const noexcept
    { return static_cast<DSCP>(traffic_class() >> 2); }

    /** Get Explicit Congestion Notification (ECN) bits */
    ECN ip_ecn() const noexcept
    { return ECN(traffic_class() & 0x3); }

    /** Get flow label */
    uint32_t flow_label() const noexcept
    { return (htonl(ver_tc_fl()) & 0x000FFFFF); }

    /** Get payload length */
    uint16_t payload_length() const noexcept
    { return ntohs(ip6_header().payload_length); }

    /** Get next header */
    Protocol next_protocol() const noexcept
    { return static_cast<Protocol>(ip6_header().next_header); }

    /** Protocol after Extension headers */
    Protocol ip_protocol() const noexcept
    {
      return ip6::parse_upper_layer_proto(ext_hdr_start(), data_end(), next_protocol());
    }

    /** Get next header */
    uint8_t next_header() const noexcept
    { return ip6_header().next_header; }

    /** Get hop limit */
    uint8_t hop_limit() const noexcept
    { return ip6_header().hop_limit; }

    /** Get source address */
    const ip6::Addr& ip_src() const noexcept
    { return ip6_header().saddr; }

    /** Get destination address */
    const ip6::Addr& ip_dst() const noexcept
    { return ip6_header().daddr; }

    /** Get IP data length. */
    uint16_t ip_data_length() const noexcept
    {
      Expects(size() and static_cast<size_t>(size()) >= sizeof(ip6::Header));
      return size() - sizeof(ip6::Header);
    }

    /** Get total data capacity of IP packet in bytes  */
    uint16_t ip_capacity() const noexcept
    { return capacity() - sizeof(ip6::Header); }

    /* This returns the IPv6 header and extension header len.
     * Note: Extension header needs to be parsed to know this */
    uint16_t ip_header_len() const noexcept
    {
      assert(payload() != nullptr);
      return payload() - layer_begin();
    }

    // IPv6 setters
    //
    /** Set IP version header field */
    void set_ip_version(uint8_t ver) noexcept
    {
      Expects(ver < 0x10);
      ip6_header().version = ver;
    }

    /** Set DSCP header bits */
    void set_ip_dscp(DSCP dscp) noexcept
    { ip6_header().traffic_class |= (static_cast<uint8_t>(dscp) << 2); }

    /** Set ECN header bits */
    void set_ip_ecn(ECN ecn) noexcept
    { ip6_header().traffic_class |= (static_cast<uint8_t>(ecn) & 0x3); }

    /** Set payload length */
    void set_ip_payload_length(uint16_t len) noexcept
    { ip6_header().payload_length = htons(len); }

    /** Set next header */
    void set_ip_next_header(uint8_t next_header) noexcept
    { ip6_header().next_header = next_header; }

    /** Set hop limit */
    void set_ip_hop_limit(uint8_t hop_limit) noexcept
    { ip6_header().hop_limit = hop_limit; }

    /** Set source address header field */
    void set_ip_src(const ip6::Addr& addr) noexcept
    { ip6_header().saddr = addr; }

    /** Set destination address header field */
    void set_ip_dst(const ip6::Addr& addr) noexcept
    { ip6_header().daddr = addr; }

    void decrement_hop_limit()
    {
      Expects(hop_limit() != 0);
      ip6_header().hop_limit--;
    }

    /** Last modifications before transmission */
    void make_flight_ready() noexcept {
      assert(ip6_header().next_header);
      set_segment_length();
    }

    void init(Protocol proto = Protocol::HOPOPT) noexcept {
      auto& hdr = ip6_header();
      hdr = {};
      hdr.hop_limit   = DEFAULT_HOP_LIMIT;
      hdr.next_header = static_cast<uint8_t>(proto);
      increment_data_end(sizeof(ip6::Header));
      set_payload_offset(sizeof(ip6::Header));
      assert(this->payload_length() == 0);
    }

    void calculate_payload_offset();

    Span ip_data() {
      return {ip_data_ptr(), ip_data_length()};
    }

    Cspan ip_data() const {
      return {ip_data_ptr(), ip_data_length()};
    }

    const uint8_t* ext_hdr_start() const
    { return layer_begin() + sizeof(ip6::Header); }

    /**
     *  Set IP6 payload length
     */
    void set_segment_length() noexcept
    { ip6_header().payload_length = htons(size() - sizeof(ip6::Header)); }

  protected:

    /** Get pointer to IP data */
    Byte* ip_data_ptr() noexcept __attribute__((assume_aligned(4)))
    {
      return payload();
    }
    const Byte* ip_data_ptr() const noexcept __attribute__((assume_aligned(4)))
    {
      return payload();
    }

  private:
    ip6::Header& ip6_header() noexcept
    { return *reinterpret_cast<ip6::Header*>(layer_begin()); }

    const ip6::Header& ip6_header() const noexcept
    { return *reinterpret_cast<const ip6::Header*>(layer_begin()); }

  }; //< PacketIP6

} //< namespace net
#endif
