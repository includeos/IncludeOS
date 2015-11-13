#pragma once

#include "../ip4.hpp"
#include "../packet.hpp"
#include "../util.hpp"
#include <cassert>

namespace net
{
  class PacketIP4 : public Packet, // might work as upcast:
                    public std::enable_shared_from_this<PacketIP4>
  {
  public:
    Ethernet::header& eth_header()
    {
      return *(Ethernet::header*) buffer();
    }
    const IP4::ip_header& ip4_header() const
    {
      return ((UDP::full_header*) buffer())->ip_hdr;
    }
    IP4::ip_header& ip4_header()
    {
      return ((UDP::full_header*) buffer())->ip_hdr;
    }
    
    const IP4::addr& src() const
    {
      return ip4_header().saddr;
    }
    void set_src(const IP4::addr& addr)
    {
      ip4_header().saddr = addr;
    }
    
    const IP4::addr& dst() const
    {
      return ip4_header().daddr;
    }
    void set_dst(const IP4::addr& addr)
    {
      ip4_header().daddr = addr;
    }
    
  };
}
