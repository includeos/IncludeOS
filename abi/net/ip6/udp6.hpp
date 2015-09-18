#pragma once

#include "../class_packet.hpp"
#include "../util.hpp"
#include "ip6.hpp"
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
    
    UDPv6(IP6::addr& local_ip)
      : localIP(local_ip) {}
    
    // set the downstream delegate
    inline void set_ip6_out(downstream del)
    {
      this->ip6_out = del;
    }
    
    // packet from IP6 layer
    int bottom(std::shared_ptr<Packet>& pckt);
    
    // packet back TO IP6 layer for transmission
    int transmit(std::shared_ptr<Packet>& pckt);
    
    void listen(port_t port, listener_t func)
    {
      listeners[port] = func;
    }
    
  protected:
    std::map<port_t, listener_t> listeners;
    // connection to IP6 layer
    downstream ip6_out;
    // this network stacks IPv6 address
    IP6::addr& localIP;
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
    
    /// TODO: move to PacketIP6 for common interface
    const IP6::addr& src() const
    {
      return ((IP6::full_header*) buffer())->ip6_hdr.src;
    }
    const IP6::addr& dst() const
    {
      return ((IP6::full_header*) buffer())->ip6_hdr.dst;
    }
    
    UDPv6::port_t src_port() const
    {
      return htons(header().src_port);
    }
    UDPv6::port_t dst_port() const
    {
      return htons(header().dst_port);
    }
    uint16_t length() const
    {
      return htons(header().length);
    }
    uint16_t checksum() const
    {
      return htons(header().chksum);
    }
    
    uint16_t data_length() const
    {
      return length() - sizeof(UDPv6::udp6_header);
    }
    char* data()
    {
      return (char*) payload() + sizeof(UDPv6::udp6_header);
    }
  };
}
