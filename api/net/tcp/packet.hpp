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

#include "common.hpp" // constants, seq_t
#include "headers.hpp"
#include "socket.hpp"

#include <sstream> // ostringstream

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

  inline Header& tcp_header() const
  { return *(Header*) ip_data(); }

  //! initializes to a default, empty TCP packet, given
  //! a valid MTU-sized buffer
  void init()
  {

    PacketIP4::init(Protocol::TCP);
    Byte* ipdata = ip_data();

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
  inline port_t src_port() const
  { return ntohs(tcp_header().source_port); }

  inline port_t dst_port() const
  { return ntohs(tcp_header().destination_port); }

  inline seq_t seq() const
  { return ntohl(tcp_header().seq_nr); }

  inline seq_t ack() const
  { return ntohl(tcp_header().ack_nr); }

  inline uint16_t win() const
  { return ntohs(tcp_header().window_size); }

  inline Socket source() const
  { return Socket{src(), src_port()}; }

  inline Socket destination() const
  { return Socket{dst(), dst_port()}; }

  inline seq_t end() const
  { return seq() + tcp_data_length(); }

  // SETTERS
  inline Packet& set_src_port(port_t p) {
    tcp_header().source_port = htons(p);
    return *this;
  }

  inline Packet& set_dst_port(port_t p) {
    tcp_header().destination_port = htons(p);
    return *this;
  }

  inline Packet& set_seq(seq_t n) {
    tcp_header().seq_nr = htonl(n);
    return *this;
  }

  inline Packet& set_ack(seq_t n) {
    tcp_header().ack_nr = htonl(n);
    return *this;
  }

  inline Packet& set_win(uint16_t size) {
    tcp_header().window_size = htons(size);
    return *this;
  }

  inline Packet& set_checksum(uint16_t checksum) {
    tcp_header().checksum = checksum;
    return *this;
  }

  inline Packet& set_source(const Socket& src) {
    set_src(src.address()); // PacketIP4::set_src
    set_src_port(src.port());
    return *this;
  }

  inline Packet& set_destination(const Socket& dest) {
    set_dst(dest.address()); // PacketIP4::set_dst
    set_dst_port(dest.port());
    return *this;
  }

  /// FLAGS / CONTROL BITS ///

  inline Packet& set_flag(Flag f) {
    tcp_header().offset_flags.whole |= htons(f);
    return *this;
  }

  inline Packet& set_flags(uint16_t f) {
    tcp_header().offset_flags.whole |= htons(f);
    return *this;
  }

  inline Packet& clear_flag(Flag f) {
    tcp_header().offset_flags.whole &= ~ htons(f);
    return *this;
  }

  inline Packet& clear_flags() {
    tcp_header().offset_flags.whole &= 0x00ff;
    return *this;
  }

  inline bool isset(Flag f) const
  { return ntohs(tcp_header().offset_flags.whole) & f; }

  //TCP::Flag flags() const { return (htons(tcp_header().offset_flags.whole) << 8) & 0xFF; }


  /// OFFSET, OPTIONS, DATA ///

  // Get the raw tcp offset, in quadruples
  inline uint8_t offset() const
  { return (uint8_t)(tcp_header().offset_flags.offset_reserved >> 4); }

  // Set raw TCP offset in quadruples
  inline void set_offset(uint8_t offset)
  { tcp_header().offset_flags.offset_reserved = (offset << 4); }

  // The actual TCP header size (including options).
  inline uint8_t tcp_header_length() const
  { return offset() * 4; }

  // The total length of the TCP segment (TCP header + data)
  uint16_t tcp_length() const
  { return tcp_header_length() + tcp_data_length(); }

  // Where data starts
  inline Byte* tcp_data()
  { return ip_data() + tcp_header_length(); }

  inline const Byte* tcp_data() const
  { return ip_data() + tcp_header_length(); }

  // Length of data in packet when header has been accounted for
  inline uint16_t tcp_data_length() const
  { return ip_data_length() - tcp_header_length(); }

  inline bool has_tcp_data() const
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
    set_offset(offset() + round_up(opt.length + Padding, 4));

    set_length(); // update
  }

  /**
   * @brief      Adds a tcp option aligned.
   *             Assumes the user knows what he's doing.
   *
   * @tparam     T          An aligned TCP option
   * @tparam     Args       construction args to option T
   */
  template <typename T, typename... Args>
  inline void add_tcp_option_aligned(Args&&... args) {
    // to avoid headache, options need to be added BEFORE any data.
    Expects(!has_tcp_data() and sizeof(T) % 4 == 0);

    // option address
    auto* addr = tcp_options()+tcp_options_length();
    // emplace the option
    new (addr) T(args...);

    // update offset
    set_offset(offset() + (sizeof(T) / 4));

    set_length(); // update
  }

  inline void clear_options() {
    // clear existing options
    // move data (if any) (??)
    set_offset(5);
    set_length(); // update
  }

  // Options
  inline uint8_t* tcp_options()
  { return (uint8_t*) tcp_header().options; }

  inline const uint8_t* tcp_options() const
  { return (const uint8_t*) tcp_header().options; }

  inline uint8_t tcp_options_length() const
  { return tcp_header_length() - sizeof(Header); }

  inline bool has_tcp_options() const
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
    std::ostringstream os;
    os << "[ S:" << source().to_string() << " D:" <<  destination().to_string()
       << " SEQ:" << seq() << " ACK:" << ack()
       << " HEAD-LEN:" << (int)tcp_header_length() << " OPT-LEN:" << (int)tcp_options_length() << " DATA-LEN:" << tcp_data_length()
       << " WIN:" << win() << " FLAGS:" << std::bitset<8>{tcp_header().offset_flags.flags}  << " ]";
    return os.str();
  }


private:
  // sets the correct length for all the protocols up to IP4
  void set_length(uint16_t newlen = 0) {
    // new total packet length
    set_data_end( ip_header_length() + tcp_header_length() + newlen );
    // update IP packet aswell - tcp data length relies on this field
    set_segment_length();
  }

}; // << class Packet

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_PACKET_HPP
