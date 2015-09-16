#pragma once

#include "../class_packet.hpp"
#include "../util.hpp"
#include "ip6.hpp"

namespace net
{
  class ICMPv6
  {
  public:
    //ICMPv6(IP6& ip6_parent)
    //  : ip6(ip6_parent) {}
    
    struct icmp6_header
    {
      uint8_t  type_;
      uint8_t  code_;
      uint16_t checksum_;
    };
    
    // packet from IP6 layer
    int bottom(std::shared_ptr<Packet>& pckt);
  };
  
  class PacketICMP6 : public Packet
  {
  public:
    inline ICMPv6::icmp6_header& header()
    {
      return *(ICMPv6::icmp6_header*) this->payload();
    }
    inline const ICMPv6::icmp6_header& header() const
    {
      return *(ICMPv6::icmp6_header*) this->payload();
    }
    
    uint8_t type() const
    {
      return header().type_;
    }
    uint8_t code() const
    {
      return header().code_;
    }
    uint16_t checksum() const
    {
      return ntohs(header().checksum_);
    }
 };
  
}
