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
#include "headers.hpp"
#include "options.hpp"

#include <net/socket.hpp>
#include <net/packet.hpp>
#include <net/iana.hpp>

namespace net::tcp {

template <typename Ptr_type> class Packet_v;
using Packet_view = Packet_v<net::Packet_ptr>;
using Packet_view_raw = Packet_v<net::Packet*>;
using Packet_view_ptr = std::unique_ptr<Packet_view>;

template <typename Ptr_type>
class Packet_v {
public:

  const Header& tcp_header() const noexcept
  { return *header; }

  // Ports etc //

  port_t src_port() const noexcept
  { return ntohs(tcp_header().source_port); }

  port_t dst_port() const noexcept
  { return ntohs(tcp_header().destination_port); }

  Socket source() const noexcept
  { return Socket{ip_src(), src_port()}; }

  Socket destination() const noexcept
  { return Socket{ip_dst(), dst_port()}; }

  Packet_v& set_src_port(port_t p) noexcept
  { tcp_header().source_port = htons(p); return *this; }

  Packet_v& set_dst_port(port_t p) noexcept
  { tcp_header().destination_port = htons(p); return *this; }

  Packet_v& set_source(const Socket& src)
  {
    set_ip_src(src.address());
    set_src_port(src.port());
    return *this;
  }

  Packet_v& set_destination(const Socket& dest)
  {
    set_ip_dst(dest.address());
    set_dst_port(dest.port());
    return *this;
  }

  // Flags //

  seq_t seq() const noexcept
  { return ntohl(tcp_header().seq_nr); }

  seq_t ack() const noexcept
  { return ntohl(tcp_header().ack_nr); }

  uint16_t win() const noexcept
  { return ntohs(tcp_header().window_size); }

  bool isset(Flag f) const noexcept
  { return ntohs(tcp_header().offset_flags.whole) & f; }

  // Get the raw tcp offset, in quadruples
  int offset() const noexcept
  { return tcp_header().offset_flags.offset_reserved >> 4; }


  Packet_v& set_seq(seq_t n) noexcept
  { tcp_header().seq_nr = htonl(n); return *this; }

  Packet_v& set_ack(seq_t n) noexcept
  { tcp_header().ack_nr = htonl(n); return *this; }

  Packet_v& set_win(uint16_t size) noexcept
  { tcp_header().window_size = htons(size); return *this; }

  Packet_v& set_flag(Flag f) noexcept
  { tcp_header().offset_flags.whole |= htons(f); return *this; }

  Packet_v& set_flags(uint16_t f) noexcept
  { tcp_header().offset_flags.whole |= htons(f); return *this; }

  Packet_v& clear_flag(Flag f) noexcept
  { tcp_header().offset_flags.whole &= ~ htons(f); return *this; }

  Packet_v& clear_flags()
  { tcp_header().offset_flags.whole &= 0x00ff; return *this; }

  // Set raw TCP offset in quadruples
  void set_offset(int offset)
  { tcp_header().offset_flags.offset_reserved = (offset & 0xF) << 4; }


  // The actual TCP header size (including options).
  auto tcp_header_length() const noexcept
  { return offset() * 4; }

  uint16_t tcp_length() const
  { return tcp_header_length() + tcp_data_length(); }

  uint16_t tcp_checksum() const noexcept
  { return tcp_header().checksum; }

  virtual uint16_t compute_tcp_checksum() const noexcept = 0;

  Packet_v& set_tcp_checksum(uint16_t checksum) noexcept
  { tcp_header().checksum = checksum; return *this; }

  void set_tcp_checksum() noexcept
  {
    tcp_header().checksum = 0;
    set_tcp_checksum(compute_tcp_checksum());
  }

  // Options //

  uint8_t* tcp_options()
  { return (uint8_t*) tcp_header().options; }

  const uint8_t* tcp_options() const
  { return (const uint8_t*) tcp_header().options; }

  int tcp_options_length() const
  { return tcp_header_length() - sizeof(Header); }

  bool has_tcp_options() const
  { return tcp_options_length() > 0; }

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
  void add_tcp_option(Args&&... args);

  /**
   * @brief      Adds a tcp option aligned.
   *             Assumes the user knows what she/he is doing.
   *
   * @tparam     T          An aligned TCP option
   * @tparam     Args       construction args to option T
   */
  template <typename T, typename... Args>
  void add_tcp_option_aligned(Args&&... args);

  const Option::opt_ts* ts_option() const noexcept
  { return ts_opt; }

  inline const Option::opt_ts* parse_ts_option() const noexcept;

  void set_ts_option(const Option::opt_ts* opt)
  { this->ts_opt = opt; }

  // Data //

  uint8_t* tcp_data()
  { return (uint8_t*)header + tcp_header_length(); }

  const uint8_t* tcp_data() const
  { return (uint8_t*)header + tcp_header_length(); }

  // Length of data in packet when header has been accounted for
  uint16_t tcp_data_length() const noexcept
  { return ip_data_length() - tcp_header_length(); }

  bool has_tcp_data() const noexcept
  { return tcp_data_length() > 0; }

  inline size_t fill(const uint8_t* buffer, size_t length);

  bool validate_length() const noexcept {
    return ip_data_length() >= tcp_header_length();
  }

  // Util //

  seq_t end() const noexcept
  { return seq() + tcp_data_length(); }

  bool is_acked_by(const seq_t ack) const noexcept
  { return ack >= (seq() + tcp_data_length()); }

  bool should_rtx() const noexcept
  { return has_tcp_data() or isset(SYN) or isset(FIN); }

  inline std::string to_string() const;

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

  Header& tcp_header() noexcept
  { return *header; }

  void set_header(uint8_t* hdr)
  { Expects(hdr != nullptr); header = reinterpret_cast<Header*>(hdr); }

  // sets the correct length for all the protocols up to IP4
  void set_length(uint16_t newlen = 0)
  { pkt->set_data_end(ip_header_length() + tcp_header_length() + newlen); }


private:
  const Option::opt_ts*   ts_opt = nullptr;

  virtual void set_ip_src(const net::Addr& addr) noexcept = 0;
  virtual void set_ip_dst(const net::Addr& addr) noexcept = 0;
  virtual net::Addr ip_src() const noexcept = 0;
  virtual net::Addr ip_dst() const noexcept = 0;

  // TODO: see if we can get rid of these virtual calls
  virtual uint16_t ip_data_length() const noexcept = 0;
  virtual uint16_t ip_header_length() const noexcept = 0;
  uint16_t ip_capacity() const noexcept
  { return pkt->capacity() - ip_header_length(); }

};

inline unsigned round_up(unsigned n, unsigned div) {
  Expects(div > 0);
  return (n + div - 1) / div;
}

template <typename Ptr_type>
template <typename T, int Padding, typename... Args>
inline void Packet_v<Ptr_type>::add_tcp_option(Args&&... args) {
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

template <typename Ptr_type>
template <typename T, typename... Args>
inline void Packet_v<Ptr_type>::add_tcp_option_aligned(Args&&... args) {
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

template <typename Ptr_type>
inline const Option::opt_ts* Packet_v<Ptr_type>::parse_ts_option() const noexcept
{
  auto* opt = this->tcp_options();
  while(opt < (uint8_t*)this->tcp_data())
  {
    auto* option = (Option*)opt;
    // zero-length options cause infinite loops (and are invalid)
    if (option->length == 0) break;
    
    switch(option->kind)
    {
      case Option::NOP: {
        opt++;
        break;
      }

      case Option::TS: {
        return reinterpret_cast<Option::opt_ts*>(option);
      }

      case Option::END: {
        return nullptr;
      }

      default:
        opt += option->length;
    }
  }

  return nullptr;
}

template <typename Ptr_type>
inline size_t Packet_v<Ptr_type>::fill(const uint8_t* buffer, size_t length)
{
  size_t rem = ip_capacity() - tcp_length();
  if(rem == 0) return 0;
  size_t total = std::min(length, rem);
  // copy from buffer to packet buffer
  memcpy(tcp_data() + tcp_data_length(), buffer, total);
  // set new packet length
  set_length(tcp_data_length() + total);
  return total;
}

template <typename Ptr_type>
inline std::string Packet_v<Ptr_type>::to_string() const
{
  char buffer[256];
  int len = snprintf(buffer, sizeof(buffer),
        "[ S:%s D:%s SEQ:%u ACK:%u HEAD-LEN:%d OPT-LEN:%d DATA-LEN:%d"
        " WIN:%u FLAGS:%#x ]",
        source().to_string().c_str(), destination().to_string().c_str(),
        seq(), ack(), tcp_header_length(), tcp_options_length(),
        tcp_data_length(), win(), tcp_header().offset_flags.flags);
  return std::string(buffer, len);
}

}
