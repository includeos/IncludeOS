// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_TCP_PACKET_HPP
#define NET_TCP_PACKET_HPP

#include <net/ip4/packet_ip4.hpp> // PacketIP4
#include <net/util.hpp> // byte ordering helpers
#include <net/socket.hpp>

#include "common.hpp" // constants, seq_t
#include "headers.hpp"

inline unsigned round_up(unsigned n, unsigned div) {
  Expects(div > 0);
  return (n + div - 1) / div;
}

namespace net {
namespace tcp {

/*
  A Wrapper for a TCP Packet. Is everything as a IP4 Packet,
  in addition to the TCP Header and functions to modify this and the control bits (FLAGS).
*/
class Packet : public PacketIP4 {
public:

  Header& tcp_header() const
  { return *(Header*) ip_data_ptr(); }

  //! initializes to a default, empty TCP packet, given
  //! a valid MTU-sized buffer
  void init()
  {
    //PacketIP4::init(Protocol::TCP);
    auto* ipdata = ip_data_ptr();

    // clear TCP header
    ((uint32_t*) ipdata)[3] = 0;
    ((uint32_t*) ipdata)[4] = 0;

    auto& hdr = *(Header*) ipdata;
    // set some default values
    hdr.window_size = htons(tcp::default_window_size);
    hdr.offset_flags.offset_reserved = (5 << 4);

    /// TODO: optimize:
    set_length();
    //set_payload(buffer() + tcp_full_header_length());
  }

  // GETTERS
  port_t src_port() const
  { return ntohs(tcp_header().source_port); }

  port_t dst_port() const
  { return ntohs(tcp_header().destination_port); }

  seq_t seq() const
  { return ntohl(tcp_header().seq_nr); }

  seq_t ack() const
  { return ntohl(tcp_header().ack_nr); }

  uint16_t win() const
  { return ntohs(tcp_header().window_size); }

  uint16_t tcp_checksum() const noexcept
  { return tcp_header().checksum; }

  Socket source() const
  { return Socket{ip_src(), src_port()}; }

  Socket destination() const
  { return Socket{ip_dst(), dst_port()}; }

  seq_t end() const
  { return seq() + tcp_data_length(); }

  // SETTERS
  Packet& set_src_port(port_t p) {
    tcp_header().source_port = htons(p);
    return *this;
  }

  Packet& set_dst_port(port_t p) {
    tcp_header().destination_port = htons(p);
    return *this;
  }

  Packet& set_seq(seq_t n) {
    tcp_header().seq_nr = htonl(n);
    return *this;
  }

  Packet& set_ack(seq_t n) {
    tcp_header().ack_nr = htonl(n);
    return *this;
  }

  Packet& set_win(uint16_t size) {
    tcp_header().window_size = htons(size);
    return *this;
  }

  Packet& set_tcp_checksum(uint16_t checksum) noexcept {
    tcp_header().checksum = checksum;
    return *this;
  }

  void set_tcp_checksum() noexcept {
    tcp_header().checksum = 0;
    set_tcp_checksum(compute_tcp_checksum());
  }

  uint16_t compute_tcp_checksum() noexcept
  { return tcp::calculate_checksum(*this); };


  Packet& set_source(const Socket& src) {
    set_ip_src(src.address().v4()); // PacketIP4::set_src
    set_src_port(src.port());
    return *this;
  }

  Packet& set_destination(const Socket& dest) {
    set_ip_dst(dest.address().v4()); // PacketIP4::set_dst
    set_dst_port(dest.port());
    return *this;
  }

  /// FLAGS / CONTROL BITS ///

  Packet& set_flag(Flag f) {
    tcp_header().offset_flags.whole |= htons(f);
    return *this;
  }

  Packet& set_flags(uint16_t f) {
    tcp_header().offset_flags.whole |= htons(f);
    return *this;
  }

  Packet& clear_flag(Flag f) {
    tcp_header().offset_flags.whole &= ~ htons(f);
    return *this;
  }

  Packet& clear_flags() {
    tcp_header().offset_flags.whole &= 0x00ff;
    return *this;
  }

  bool isset(Flag f) const
  { return ntohs(tcp_header().offset_flags.whole) & f; }

  //TCP::Flag flags() const { return (htons(tcp_header().offset_flags.whole) << 8) & 0xFF; }


  /// OFFSET, OPTIONS, DATA ///

  // Get the raw tcp offset, in quadruples
  int offset() const
  { return tcp_header().offset_flags.offset_reserved >> 4; }

  // Set raw TCP offset in quadruples
  void set_offset(int offset)
  { tcp_header().offset_flags.offset_reserved = (offset & 0xF) << 4; }

  // The actual TCP header size (including options).
  auto tcp_header_length() const
  { return offset() * 4; }

  // The total length of the TCP segment (TCP header + data)
  uint16_t tcp_length() const
  { return tcp_header_length() + tcp_data_length(); }

  // Where data starts
  Byte* tcp_data()
  { return ip_data_ptr() + tcp_header_length(); }

  const Byte* tcp_data() const
  { return ip_data_ptr() + tcp_header_length(); }

  // Length of data in packet when header has been accounted for
  uint16_t tcp_data_length() const
  { return ip_data_length() - tcp_header_length(); }

  bool has_tcp_data() const
  { return tcp_data_length() > 0; }

  /**
   * @brief      Adds a tcp option.
   *
   * @todo       It's probably a better idea to make the option include
   *             the padding for it to be aligned, and avoid two mem operations
   *
   * @tparam     T          TCP Option
   * @tparam     Padding    padding in bytes to be put infront of the option
   * @tparam     Args       construction args to option T
   */
  template <typename T, int Padding = 0, typename... Args>
  inline void add_tcp_option(Args&&... args) {
    // to avoid headache, options need to be added BEFORE any data.
    assert(!has_tcp_data());
    struct NOP {
      uint8_t kind{0x01};
    };
    // option address
    auto* addr = tcp_options()+tcp_options_length();
    // if to use pre padding
    if(Padding)
      new (addr) NOP[Padding];

    // emplace the option after pre padding
    const auto& opt = *(new (addr + Padding) T(args...));

    // find number of NOP to pad with
    const auto nops = (opt.length + Padding) % 4;
    if(nops) {
      new (addr + Padding + opt.length) NOP[nops];
    }

    // update offset
    auto newoffset = offset() + round_up(opt.length + Padding, 4);
    if (UNLIKELY(newoffset > 0xF)) {
      throw std::runtime_error("Too many TCP options");
    }
    set_offset(newoffset);

    set_length(); // update
  }

  /**
   * @brief      Adds a tcp option aligned.
   *             Assumes the user knows what she/he is doing.
   *
   * @tparam     T          An aligned TCP option
   * @tparam     Args       construction args to option T
   */
  template <typename T, typename... Args>
  inline void add_tcp_option_aligned(Args&&... args) {
    // to avoid headache, options need to be added BEFORE any data.
    Expects(!has_tcp_data());

    // option address
    auto* addr = tcp_options()+tcp_options_length();
    // emplace the option
    auto& opt = *(new (addr) T(args...));

    // update offset
    auto newoffset = offset() + round_up(opt.size(), 4);

    set_offset(newoffset);
    if (UNLIKELY(newoffset > 0xF)) {
      throw std::runtime_error("Too many TCP options");
    }
    set_length(); // update
  }

  void clear_options() {
    // clear existing options
    if (UNLIKELY(tcp_data_length() > 0)) {
      throw std::runtime_error("Can't clear options on TCP packet with data");
    }
    set_offset(5);
    set_length(); // update
  }

  // Options
  uint8_t* tcp_options()
  { return (uint8_t*) tcp_header().options; }

  const uint8_t* tcp_options() const
  { return (const uint8_t*) tcp_header().options; }

  int tcp_options_length() const
  { return tcp_header_length() - sizeof(Header); }

  bool has_tcp_options() const
  { return tcp_options_length() > 0; }


  //! assuming the packet has been properly initialized,
  //! this will fill bytes from @buffer into this packets buffer,
  //! then return the number of bytes written. buffer is unmodified
  size_t fill(const uint8_t* buffer, size_t length) {
    size_t rem = ip_capacity() - tcp_length();
    if(rem == 0) return 0;
    size_t total = std::min(length, rem);
    // copy from buffer to packet buffer
    memcpy(tcp_data() + tcp_data_length(), buffer, total);
    // set new packet length
    set_length(tcp_data_length() + total);
    return total;
  }

  /// HELPERS ///

  bool is_acked_by(const seq_t ack) const
  { return ack >= (seq() + tcp_data_length()); }

  bool should_rtx() const
  { return has_tcp_data() or isset(SYN) or isset(FIN); }

  std::string to_string() const {
    char buffer[512];
    int len = snprintf(buffer, sizeof(buffer),
          "[ S:%s D:%s SEQ:%u ACK:%u HEAD-LEN:%d OPT-LEN:%d DATA-LEN:%d"
          " WIN:%u FLAGS:%#x ]",
          source().to_string().c_str(), destination().to_string().c_str(),
          seq(), ack(), tcp_header_length(), tcp_options_length(),
          tcp_data_length(), win(), tcp_header().offset_flags.flags);
    return std::string(buffer, len);
  }


private:
  // sets the correct length for all the protocols up to IP4
  void set_length(uint16_t newlen = 0) {
    // new total packet length
    set_data_end( ip_header_length() + tcp_header_length() + newlen );

  }

}; // << class Packet

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_PACKET_HPP
