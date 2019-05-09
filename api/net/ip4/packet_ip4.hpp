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

#include "header.hpp"
#include <cassert>
#include <net/packet.hpp>
#include <net/inet_common.hpp>

namespace net {

  /** IPv4 packet. */
  class PacketIP4 : public Packet {
  public:
    static constexpr int DEFAULT_TTL = 64;

    using Span = gsl::span<Byte>;
    using Cspan = gsl::span<const Byte>;

    //
    // IP header getters
    //

    /** Get IP protocol version field. Must be 4 (RFC 1122) */
    uint8_t ip_version() const noexcept
    { return (ip_header().version_ihl >> 4) & 0xf; }

    bool is_ipv4() const noexcept
    { return (ip_header().version_ihl & 0xf0) == 0x40; }

    /** Get IP header length field as-is. */
    uint8_t ip_ihl() const noexcept
    { return (ip_header().version_ihl & 0xf); }

    /** Get IP header length field in bytes. */
    uint8_t ip_header_length() const noexcept
    { return (ip_header().version_ihl & 0xf) * 4; }

    /** Get Differentiated Services Code Point (DSCP)*/
    DSCP ip_dscp() const noexcept
    { return static_cast<DSCP>(ip_header().ds_ecn >> 2); }

    /** Get Explicit Congestion Notification (ECN) bits */
    ECN ip_ecn() const noexcept
    { return ECN(ip_header().ds_ecn & 0x3); }

    /** Get total length header field */
    uint16_t ip_total_length() const noexcept
    { return ntohs(ip_header().tot_len); }

    /** Get ID header field */
    uint16_t ip_id() const noexcept
    { return ntohs(ip_header().id); }

    /** Get IP header flags */
    ip4::Flags ip_flags() const noexcept
    { return static_cast<ip4::Flags>(ntohs(ip_header().frag_off_flags) >> 13); }

    /** Get Fragment offset field */
    uint16_t ip_frag_offs() const noexcept
    { return ntohs(ip_header().frag_off_flags) & 0x1fff; }

    /** Get Time-To-Live field */
    uint8_t ip_ttl() const noexcept
    { return ip_header().ttl; }

    /** Get protocol field value */
    Protocol ip_protocol() const noexcept
    { return static_cast<Protocol>(ip_header().protocol); }

    /** Get the IP header checksum field as-is */
    uint16_t ip_checksum() const noexcept
    { return ip_header().check; }

    /** Get source address */
    const ip4::Addr& ip_src() const noexcept
    { return ip_header().saddr; }

    /** Get destination address */
    const ip4::Addr& ip_dst() const noexcept
    { return ip_header().daddr; }

    /** Get IP data length. */
    uint16_t ip_data_length() const noexcept
    {
      //Expects(size() and static_cast<size_t>(size()) >= sizeof(ip4::Header));
      return size() - ip_header_length();
    }

    /** Adjust packet size to match IP header's tot_len in case of padding */
    void adjust_size_from_header() {
      auto ip_len = ip_total_length();
      if (UNLIKELY(size() > ip_len)) {
        set_data_end(ip_len);
      }
    }

    /** Get total data capacity of IP packet in bytes  */
    uint16_t ip_capacity() const noexcept
    { return capacity() - ip_header_length(); }

    /** Compute IP header checksum on header as-is */
    uint16_t compute_ip_checksum() noexcept
    { return net::checksum(&ip_header(), ip_header_length()); };


    //
    // IP header setters
    //

    /** Set IP version header field */
    void set_ip_version(uint8_t ver) noexcept
    {
      Expects(ver < 0x10);
      ip_header().version_ihl &= 0x0F;
      ip_header().version_ihl |= ver << 4;
    }

    /** Set IP header length field */
    void set_ihl(uint8_t ihl) noexcept
    {
      Expects(ihl < 0x10);
      ip_header().version_ihl &= 0xF0;
      ip_header().version_ihl |= ihl;
    }

    /** Set IP header lenght field, in bytes */
    void set_ip_header_length(uint8_t bytes) noexcept
    { set_ihl(bytes / 4); }

    /** Set DSCP header bits */
    void set_ip_dscp(DSCP dscp) noexcept
    { ip_header().ds_ecn |= (static_cast<uint8_t>(dscp) << 2); }

    /** Set ECN header bits */
    void set_ip_ecn(ECN ecn) noexcept
    { ip_header().ds_ecn |= (static_cast<uint8_t>(ecn) & 0x3); }

    /** Set total length header field */
    void set_ip_total_length(uint16_t len) noexcept
    { ip_header().tot_len = htons(len); }

    /** Set ID header field */
    void set_ip_id(uint16_t i) noexcept
    { ip_header().id = htons(i); }

    /** Set flags field */
    void set_ip_flags(ip4::Flags f)
    {
      ip_header().frag_off_flags |= static_cast<uint16_t>(f) << 13;
      ip_header().frag_off_flags = htons(ip_header().frag_off_flags);
    }

    /** Set fragment offset header field */
    void set_ip_frag_offs(uint16_t offs)
    {
      Expects(offs < 0x2000);
      ip_header().frag_off_flags |= htons(offs) >> 3;
    }

    /** Set total length header field */
    void set_ip_ttl(uint8_t ttl) noexcept
    { ip_header().ttl = ttl; }

    /**
     * @brief      Decrement Time-To-Live by 1 and adjust the checksum.
     */
    void decrement_ttl()
    {
      Expects(ip_ttl() != 0);
      ip_header().ttl--;
      // RFC 1141 p. 1
      uint16_t sum = ntohs(ip_header().check + htons(0x100)); // increment checksum high byte
      ip_header().check = htons(sum + (sum>>16)); // add carry
    }

    /** Set protocol header field */
    void set_protocol(Protocol p) noexcept
    { ip_header().protocol = static_cast<uint8_t>(p); }

    /** Set IP header checksum field directly */
    void set_ip_checksum(uint16_t sum) noexcept
    { ip_header().check = sum; }

    /** Calculate and set IP header checksum field */
    void set_ip_checksum() noexcept {
      auto& hdr = ip_header();
      hdr.check = 0;
      hdr.check = net::checksum(&hdr, ip_header_length());
    }

    /** Set source address header field */
    void set_ip_src(const ip4::Addr& addr) noexcept
    { ip_header().saddr = addr; }

    /** Set destination address header field */
    void set_ip_dst(const ip4::Addr& addr) noexcept
    { ip_header().daddr = addr; }

    /**
     * Set size of data contained in IP datagram.
     * @note : does not modify IP header
     **/
    void set_ip_data_length(uint16_t length) noexcept
    {
      Expects(sizeof(ip4::Header) + length <= (size_t) capacity());
      set_data_end(sizeof(ip4::Header) + length);
    }

    /** Last modifications before transmission */
    void make_flight_ready() noexcept {
      assert( ip_header().protocol );
      set_segment_length();
      set_ip_checksum();
    }

    void init(Protocol proto = Protocol::HOPOPT) noexcept {
      Expects(size() == 0);
      auto& hdr = ip_header();
      hdr = {};
      hdr.tot_len        = 0x1400; // Big-endian 20
      hdr.ttl            = DEFAULT_TTL;
      hdr.protocol       = static_cast<uint8_t>(proto);
      increment_data_end(sizeof(ip4::Header));
    }

    Span ip_data() {
      return {ip_data_ptr(), ip_data_length()};
    }

    Cspan ip_data() const {
      return {ip_data_ptr(), ip_data_length()};
    }

    bool validate_length() const noexcept {
      return this->size() == ip_header_length() + ip_data_length();
    }

  protected:

    /** Get pointer to IP data */
    Byte* ip_data_ptr() noexcept __attribute__((assume_aligned(4)))
    {
      return layer_begin() + ip_header_length();
    }

    const Byte* ip_data_ptr() const noexcept __attribute__((assume_aligned(4)))
    {
      return layer_begin() + ip_header_length();
    }

  private:

    /**
     *  Set IP4 header length
     *  Inferred from packet size
     */
    void set_segment_length() noexcept
    { ip_header().tot_len = htons(size()); }

    const ip4::Header& ip_header() const noexcept
    { return *reinterpret_cast<const ip4::Header*>(layer_begin()); }

    ip4::Header& ip_header() noexcept
    { return *reinterpret_cast<ip4::Header*>(layer_begin()); }

  }; //< class PacketIP4
} //< namespace net

#endif //< IP4_PACKET_IP4_HPP
