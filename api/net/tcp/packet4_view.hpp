
#pragma once

#include <net/tcp/packet_view.hpp>
#include <net/ip4/packet_ip4.hpp>

namespace net::tcp {

class Packet4_view : public Packet_view {
public:
  Packet4_view(std::unique_ptr<PacketIP4> ptr)
    : Packet_view(std::move(ptr))
  {
    Expects(packet().is_ipv4());
    header = reinterpret_cast<Header*>(packet().ip_data().data());
  }

  uint16_t compute_tcp_checksum() noexcept override
  { return calculate_checksum6(*this); }

private:
  PacketIP4& packet() noexcept
  { return static_cast<PacketIP4&>(*pkt); }

  const PacketIP4& packet() const noexcept
  { return static_cast<PacketIP4&>(*pkt); }

  void set_ip_src(const net::Addr& addr) noexcept override
  { packet().set_ip_src(addr.v4()); }

  void set_ip_dst(const net::Addr& addr) noexcept override
  { packet().set_ip_dst(addr.v4()); }

  net::Addr ip_src() const noexcept override
  { return packet().ip_src(); }

  net::Addr ip_dst() const noexcept override
  { return packet().ip_dst(); }

   uint16_t ip_data_length() const noexcept override
   { return packet().ip_data_length(); }
};

}
