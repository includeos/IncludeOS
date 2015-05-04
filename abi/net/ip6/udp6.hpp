#pragma once

#include <x86intrin.h>
#include <net/class_packet.hpp>
#include <net/util.hpp>
#include <map>

namespace net
{
  class PacketUDP6;
  
  class UDPv6
  {
  public:
    typedef uint16_t port_t;
    typedef int (*listener_t)(std::shared_ptr<PacketUDP6>& pckt);
    
    struct udp6_header
    {
      uint16_t src_port;
      uint16_t dst_port;
      uint16_t length;
      uint16_t chksum;
    };
    
    // packet from IP6 layer
    int bottom(std::shared_ptr<Packet>& pckt);
    
  protected:
    std::map<port_t, listener_t> listeners;
    
    friend class PacketUDP6;
  };
  
  class PacketUDP6 : public Packet
  {
  public:
    UDPv6::udp6_header& header()
    {
      return *(UDPv6::udp6_header*) payload();
    }
    const UDPv6::udp6_header& header() const
    {
      return *(UDPv6::udp6_header*) payload();
    }
    
    UDPv6::port_t getSourcePort() const
    {
      return htons(header().src_port);
    }
    UDPv6::port_t getDestPort() const
    {
      return htons(header().src_port);
    }
    uint16_t getLength() const
    {
      return htons(header().length);
    }
    uint16_t getChecksum() const
    {
      return htons(header().chksum);
    }
    
    const char* getData() const
    {
      return (char*) payload() + sizeof(UDPv6::udp6_header);
    }
    
  };
}