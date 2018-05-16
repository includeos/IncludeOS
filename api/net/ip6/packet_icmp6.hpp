
// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <cstdint>
#include <gsl/span>
#include <net/ip6/packet_ip6.hpp>
#include <net/ip6/icmp6_error.hpp>
#include <net/ip6/ip6.hpp>

namespace net {

#define NEIGH_ADV_ROUTER   0x1
#define NEIGH_ADV_SOL      0x2
#define NEIGH_ADV_OVERRIDE 0x4

namespace icmp6 {

  class Packet {

    struct IdSe {
      uint16_t identifier;
      uint16_t sequence;
    };

    struct Header {
      Type  type;
      uint8_t  code;
      uint16_t checksum;
      union {
        struct IdSe  idse;
        uint32_t     reserved;
        uint32_t     rso_flags;
      };
      uint8_t  payload[0];
    }__attribute__((packed));

    Header& header()
    { return *reinterpret_cast<Header*>(pckt_->layer_begin() + pckt_->ip_header_len()); }

    const Header& header() const
    { return *reinterpret_cast<Header*>(pckt_->layer_begin() + pckt_->ip_header_len()); }

    struct pseudo_header
    {
      IP6::addr src;
      IP6::addr dst;
      uint32_t  len;
      uint8_t   zeros[3];
      uint8_t   next;
    } __attribute__((packed));

    struct nd_options_header {
        uint8_t type;
        uint8_t len;
    } __attribute__((packed));

    struct RouterSol
    {
      uint8_t  options[0];
    } __attribute__((packed));

    struct RouterAdv
    {
        uint32_t reachable_time;
        uint32_t retrans_timer;
    } __attribute__((packed));

    struct RouterRedirect
    {
      IP6::addr target;
      IP6::addr dest;
      uint8_t  options[0];
    } __attribute__((packed));

    struct NeighborSol
    {
      IP6::addr target;
      uint8_t   options[0];

      IP6::addr get_target()
      { return target; }

    } __attribute__((packed));

    struct NeighborAdv
    {
      IP6::addr target;
      uint8_t   options[0];

      void set_target(IP6::addr tar)
      { target = tar; }
    } __attribute__((packed));

  public:

    using Span = gsl::span<uint8_t>;

    static constexpr size_t header_size()
    { return sizeof(Header); }

    Type type() const noexcept
    { return header().type; }

    uint8_t code() const noexcept
    { return header().code; }

    uint16_t checksum() const noexcept
    { return header().checksum; }

    uint16_t id() const noexcept
    { return header().idse.identifier; }

    uint16_t sequence() const noexcept
    { return header().idse.sequence; }

    /**
     *  Where the payload of an ICMP packet starts, calculated from the start of the IP header
     *  The payload of an ICMP error message packet contains the original packet sent that caused an
     *  ICMP error to occur (the original IP header and 8 bytes of the original packet's data)
     */
    int payload_index()
    { return pckt_->ip_header_len() + header_size(); }

    Span payload()
    { return {&(header().payload[0 + payload_offset_]), pckt_->data_end() - &(header().payload[0 + payload_offset_]) }; }

    /** Several ICMP messages require the payload to be the header and 64 bits of the
     *  data of the original datagram
     */
    Span header_and_data()
    { return {pckt_->layer_begin(), pckt_->ip_header_len() + 8}; }

    RouterSol& router_sol()
    { return *reinterpret_cast<RouterSol*>(&(header().payload[0])); }

    RouterAdv& router_adv()
    { return *reinterpret_cast<RouterAdv*>(&(header().payload[0])); }

    RouterRedirect& router_redirect()
    { return *reinterpret_cast<RouterRedirect*>(&(header().payload[0])); }

    NeighborSol& neighbor_sol()
    { return *reinterpret_cast<NeighborSol*>(&(header().payload[0])); }

    NeighborAdv& neighbor_adv()
    { return *reinterpret_cast<NeighborAdv*>(&(header().payload[0])); }

    void set_type(Type t) noexcept
    { header().type = t; }

    void set_code(uint8_t c) noexcept
    { header().code = c; }

    void set_id(uint16_t s) noexcept
    { header().idse.identifier = s; }

    void set_sequence(uint16_t s) noexcept
    { header().idse.sequence = s; }

    void set_reserved(uint32_t s) noexcept
    { header().reserved = s; }

    /**
     * RFC 792 Parameter problem f.ex.: error (Pointer) is placed in the first byte after checksum
     * (identifier and sequence is not used when pointer is used)
     */
    void set_pointer(uint8_t error)
    { header().idse.identifier = error; }

    uint16_t compute_checksum() const noexcept
    {
        uint16_t datalen = ip().payload_length();
        pseudo_header phdr;

        // ICMP checksum is done with a pseudo header
        // consisting of src addr, dst addr, message length (32bits)
        // 3 zeroes (8bits each) and id of the next header
        phdr.src = ip().ip_src();
        phdr.dst = ip().ip_dst();
        phdr.len = htonl(datalen);
        phdr.zeros[0] = 0;
        phdr.zeros[1] = 0;
        phdr.zeros[2] = 0;
        phdr.next = ip().next_header();
        assert(phdr.next == 58); // ICMPv6

        /**
           RFC 4443
           2.3. Message Checksum Calculation

           The checksum is the 16-bit one's complement of the one's complement
           sum of the entire ICMPv6 message, starting with the ICMPv6 message
           type field, and prepended with a "pseudo-header" of IPv6 header
           fields, as specified in [IPv6, Section 8.1].  The Next Header value
           used in the pseudo-header is 58.  (The inclusion of a pseudo-header
           in the ICMPv6 checksum is a change from IPv4; see [IPv6] for the
           rationale for this change.)

           For computing the checksum, the checksum field is first set to zero.
        **/
        union
        {
          uint32_t whole;
          uint16_t part[2];
        } sum;
        sum.whole = 0;

        // compute sum of pseudo header
        uint16_t* it = (uint16_t*) &phdr;
        uint16_t* it_end = it + sizeof(pseudo_header) / 2;

        while (it < it_end)
          sum.whole += *(it++);

        // compute sum of data
        it = (uint16_t*) &header();
        it_end = it + datalen / 2;

        while (it < it_end)
          sum.whole += *(it++);

        // odd-numbered case
        if (datalen & 1)
          sum.whole += *(uint8_t*) it;

        return ~(sum.part[0] + sum.part[1]);
    }

    void set_checksum() {
      ip().set_segment_length();
      header().checksum = 0;
      header().checksum = compute_checksum();;
    }

    void set_neighbor_adv_flag(uint32_t flag)
    { header().rso_flags = htonl(flag << 28); }

    void set_ndp_options_header(uint8_t type, uint8_t len)
    {
        struct nd_options_header ndo;
        ndo.type = type;
        ndo.len = len;
        set_payload({reinterpret_cast<uint8_t*> (&ndo), sizeof ndo});
    }

    void set_payload(Span new_load)
    {
      pckt_->set_data_end(pckt_->ip_header_len() + header_size() + payload_offset_ + new_load.size());
      memcpy(payload().data(), new_load.data(), payload().size());
      payload_offset_ += payload().size();
    }

    /** Get the underlying IP packet */
    IP6::IP_packet& ip() const
    { return *pckt_.get(); }

    /** Construct from existing packet **/
    Packet(IP6::IP_packet_ptr pckt)
      : pckt_{ std::move(pckt) }, payload_offset_{0}
    {  }

    /** Provision fresh packet from factory **/
    Packet(IP6::IP_packet_factory create)
      : pckt_{create(Protocol::ICMPv6)}, payload_offset_{0}
    {
      pckt_->increment_data_end(sizeof(Header));
    }

    /** Release packet pointer **/
    IP6::IP_packet_ptr release()
    { return std::move(pckt_); }

  private:
    IP6::IP_packet_ptr pckt_;
    uint16_t payload_offset_;
  };
}
}
