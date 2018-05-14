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
#include <net/ip4/packet_ip4.hpp>
#include <net/ip4/ip4.hpp> // please remove me

namespace net {
namespace icmp4 {

  class Packet {

    struct Header {
      Type  type;
      uint8_t  code;
      uint16_t checksum;
      uint16_t identifier;
      uint16_t sequence;
      uint8_t  payload[0];
    }__attribute__((packed));

    Header& header()
    { return *reinterpret_cast<Header*>(pckt_->layer_begin() + sizeof(IP4::header)); }

    const Header& header() const
    { return *reinterpret_cast<Header*>(pckt_->layer_begin() + sizeof(IP4::header)); }

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
    { return header().identifier; }

    uint16_t sequence() const noexcept
    { return header().sequence; }

    /**
     *  Where the payload of an ICMP packet starts, calculated from the start of the IP header
     *  The payload of an ICMP error message packet contains the original packet sent that caused an
     *  ICMP error to occur (the original IP header and 8 bytes of the original packet's data)
     */
    int payload_index()
    { return pckt_->ip_header_length() + header_size(); }

    Span payload()
    { return {&(header().payload[0]), pckt_->data_end() - &(header().payload[0]) }; }

    /** Several ICMP messages require the payload to be the header and 64 bits of the
     *  data of the original datagram
     */
    Span header_and_data()
    { return {pckt_->layer_begin(), pckt_->ip_header_length() + 8}; }

    void set_type(Type t) noexcept
    { header().type = t; }

    void set_code(uint8_t c) noexcept
    { header().code = c; }

    void set_id(uint16_t s) noexcept
    { header().identifier = s; }

    void set_sequence(uint16_t s) noexcept
    { header().sequence = s; }

    /**
     * RFC 792 Parameter problem f.ex.: error (Pointer) is placed in the first byte after checksum
     * (identifier and sequence is not used when pointer is used)
     */
    void set_pointer(uint8_t error)
    { header().identifier = error; }

    uint16_t compute_checksum() const noexcept
    {
      return net::checksum(reinterpret_cast<const uint16_t*>(&header()),
                           pckt_->data_end() - reinterpret_cast<const uint8_t*>(&header()));
    }

    void set_checksum() {
      header().checksum = 0;
      header().checksum = compute_checksum();;
    }

    void set_payload(Span new_load)
    {
      pckt_->set_data_end(pckt_->ip_header_length() + header_size() + new_load.size());
      memcpy(payload().data(), new_load.data(), payload().size());
    }

    /** Get the underlying IP packet */
    IP4::IP_packet& ip()
    { return *pckt_.get(); }

    /** Construct from existing packet **/
    Packet(IP4::IP_packet_ptr pckt)
      : pckt_{ std::move(pckt) }
    {  }

    /** Provision fresh packet from factory **/
    Packet(IP4::IP_packet_factory create)
      : pckt_{create(Protocol::ICMPv4)}
    {
      pckt_->increment_data_end(sizeof(Header));
    }

    /** Release packet pointer **/
    IP4::IP_packet_ptr release()
    { return std::move(pckt_); }

  private:
    IP4::IP_packet_ptr pckt_;
  };
}
}
