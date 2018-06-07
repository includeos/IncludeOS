
#pragma once

#include <net/tcp/common.hpp>
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

  port_t src_port() const noexcept
  { return ntohs(tcp_header().source_port); }

  port_t dst_port() const noexcept
  { return ntohs(tcp_header().destination_port); }

  Socket source() const noexcept
  { return Socket{ip_src(), src_port()}; }

  Socket destination() const noexcept
  { return Socket{ip_dst(), dst_port()}; }


  // Get the raw tcp offset, in quadruples
  int offset() const
  { return tcp_header().offset_flags.offset_reserved >> 4; }

  // The actual TCP header size (including options).
  auto tcp_header_length() const
  { return offset() * 4; }

  // Length of data in packet when header has been accounted for
  uint16_t tcp_data_length() const
  { return ip_data_length() - tcp_header_length(); }



  net::Packet_ptr release()
  {
    Expects(pkt != nullptr && "Packet ptr is already null");
    return std::move(pkt);
  }

  virtual uint16_t compute_tcp_checksum() noexcept = 0;

protected:
  net::Packet_ptr   pkt;
  Header*           header = nullptr;

  virtual void set_ip_src(const net::Addr& addr) noexcept = 0;
  virtual void set_ip_dst(const net::Addr& addr) noexcept = 0;
  virtual net::Addr ip_src() const noexcept = 0;
  virtual net::Addr ip_dst() const noexcept = 0;
  // TODO: see if we can get rid of this virtual call
  virtual uint16_t ip_data_length() const noexcept = 0;
};

}
