#pragma once

#include "../class_packet.hpp"
#include "../util.hpp"
#include "ip6.hpp"

namespace net
{
  class ICMPv6
  {
  public:
    ICMPv6(IP6::addr& local_ip)
      : localIP(local_ip) {}
    
    struct icmp6_header
    {
      uint8_t  type_;
      uint8_t  code_;
      uint16_t checksum_;
    };
    
    // handles ICMP type 128 (echo requests)
    void echo_request(std::shared_ptr<Packet>& pckt);
    
    // packet from IP6 layer
    int bottom(std::shared_ptr<Packet>& pckt);
    
    // set the downstream delegate
    inline void set_ip6_out(downstream del)
    {
      this->ip6_out = del;
    }
    
  private:
    // connection to IP6 layer
    downstream ip6_out;
    // IP6 instance
    IP6::addr localIP;
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
