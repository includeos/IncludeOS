
#pragma once

#include "common.hpp"
#include "options.hpp"

namespace net::tcp {

class Packet_view {
public:
  Packet_view(net::Packet_ptr ptr)
    : pkt{std::move(ptr)}
  {
    Expects(pkt != nullptr);
  }

  Header& tcp_header() noexcept
  { return *header; }

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

  // Flags //

  seq_t seq() const noexcept
  { return ntohl(tcp_header().seq_nr); }

  seq_t ack() const noexcept
  { return ntohl(tcp_header().ack_nr); }

  uint16_t win() const
  { return ntohs(tcp_header().window_size); }

  bool isset(Flag f) const noexcept
  { return ntohs(tcp_header().offset_flags.whole) & f; }

  // Get the raw tcp offset, in quadruples
  int offset() const noexcept
  { return tcp_header().offset_flags.offset_reserved >> 4; }

  // The actual TCP header size (including options).
  auto tcp_header_length() const noexcept
  { return offset() * 4; }

  // Data //

  // Where data starts
  uint8_t* tcp_data()
  { return (uint8_t*)header + tcp_header_length(); }

  const uint8_t* tcp_data() const
  { return (uint8_t*)header + tcp_header_length(); }

  // Length of data in packet when header has been accounted for
  uint16_t tcp_data_length() const noexcept
  { return ip_data_length() - tcp_header_length(); }

  bool has_tcp_data() const noexcept
  { return tcp_data_length() > 0; }


  // Options //

  uint8_t* tcp_options()
  { return (uint8_t*) tcp_header().options; }

  const uint8_t* tcp_options() const
  { return (const uint8_t*) tcp_header().options; }

  int tcp_options_length() const
  { return tcp_header_length() - sizeof(Header); }

  bool has_tcp_options() const
  { return tcp_options_length() > 0; }

  const Option::opt_ts* ts_option() const noexcept
  { return ts_opt; }

  inline const Option::opt_ts* parse_ts_option() noexcept;


  // View operations
  net::Packet_ptr release()
  {
    Expects(pkt != nullptr && "Packet ptr is already null");
    return std::move(pkt);
  }

  const net::Packet_ptr& packet_ptr() const noexcept
  { return pkt; }

  virtual uint16_t compute_tcp_checksum() noexcept = 0;

protected:
  net::Packet_ptr   pkt;

  void set_header(uint8_t* hdr)
  { Expects(hdr != nullptr); header = reinterpret_cast<Header*>(hdr); }

private:
  Header*           header = nullptr;
  Option::opt_ts*   ts_opt = nullptr;

  virtual void set_ip_src(const net::Addr& addr) noexcept = 0;
  virtual void set_ip_dst(const net::Addr& addr) noexcept = 0;
  virtual net::Addr ip_src() const noexcept = 0;
  virtual net::Addr ip_dst() const noexcept = 0;
  // TODO: see if we can get rid of this virtual call
  virtual uint16_t ip_data_length() const noexcept = 0;
};

// assumes the packet contains no other options.
inline const Option::opt_ts* Packet_view::parse_ts_option() noexcept
{
  auto* opt = this->tcp_options();
  // TODO: improve by iterate option instead of byte (see Connection::parse_options)
  while(((Option*)opt)->kind == Option::NOP and opt < (uint8_t*)this->tcp_data())
    opt++;

  if(((Option*)opt)->kind == Option::TS)
    this->ts_opt = (Option::opt_ts*)opt;

  return this->ts_opt;
}

}
