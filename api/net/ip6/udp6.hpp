#pragma once

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
    //typedef int (*listener_t)(std::shared_ptr<PacketUDP6>& pckt);
    typedef std::function<int(std::shared_ptr<PacketUDP6>& pckt)> listener_t;
    
    struct header
    {
      uint16_t src_port;
      uint16_t dst_port;
      uint16_t length;
      uint16_t chksum;
    } __attribute__((packed));
    
    struct pseudo_header
    {
      IP6::addr src;
      IP6::addr dst;
      uint8_t   zero;
      uint8_t   protocol;
      uint16_t  length;
    } __attribute__((packed));
    
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
    int transmit(std::shared_ptr<PacketUDP6>& pckt);
    
    // register a listener function on @port
    void listen(port_t port, listener_t func)
    {
      listeners[port] = func;
    }
    
    // creates a new packet to be sent over the ether
    std::shared_ptr<PacketUDP6> create(
        Ethernet::addr ether_dest, const IP6::addr& dest, port_t port);
    
  private:
    std::map<port_t, listener_t> listeners;
    // connection to IP6 layer
    downstream ip6_out;
    // this network stacks IPv6 address
    IP6::addr& localIP;
  };
  
  class PacketUDP6 : public PacketIP6
  {
  public:
    UDPv6::header& header()
    {
      return *(UDPv6::header*) payload();
    }
    const UDPv6::header& cheader() const
    {
      return *(UDPv6::header*) payload();
    }
    
    UDPv6::port_t src_port() const
    {
      return htons(cheader().src_port);
    }
    UDPv6::port_t dst_port() const
    {
      return htons(cheader().dst_port);
    }
    uint16_t length() const
    {
      return htons(cheader().length);
    }
    uint16_t checksum() const
    {
      return htons(cheader().chksum);
    }
    
    void set_length(uint16_t newlen)
    {
      // new total UDPv6 payload length
      header().length = htons(sizeof(UDPv6::header) + newlen);
      // new total IPv6 payload length
      ip6_header().set_size(
          sizeof(IP6::header) + sizeof(UDPv6::header) + newlen);
      // new total packet length
      _len = sizeof(IP6::full_header) + sizeof(UDPv6::header) + newlen;
    }
    
    // generates a new checksum onto this UDPv6 packet
    // returns the new generated checksum after modifying packet
    uint16_t gen_checksum();
    
    uint16_t data_length() const
    {
      return length() - sizeof(UDPv6::header);
    }
    char* data()
    {
      return (char*) payload() + sizeof(UDPv6::header);
    }
    const char* data() const
    {
      return (const char*) payload() + sizeof(UDPv6::header);
    }
  };
}
