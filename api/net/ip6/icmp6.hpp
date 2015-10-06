#pragma once

#include "../util.hpp"
#include "ip6.hpp"

namespace net
{
  class PacketICMP6;
  
  class ICMPv6
  {
  public:
    static const int ECHO_REQUEST = 128;
    static const int ECHO_REPLY   = 129;
    typedef uint8_t type_t;
    typedef int (*handler_t)(ICMPv6&, std::shared_ptr<PacketICMP6>&);
    
    ICMPv6(IP6::addr& ip6);
    
    struct header
    {
      uint8_t  type;
      uint8_t  code;
      uint16_t checksum;
    } __attribute__((packed));
    
    struct pseudo_header
    {
      IP6::addr src;
      IP6::addr dst;
      uint32_t  len;
      uint8_t   zeros[3];
      uint8_t   next;
    } __attribute__((packed));
    
    struct echo_header
    {
      uint8_t  type;
      uint8_t  code;
      uint16_t checksum;
      uint16_t identifier;
      uint16_t sequence;
      uint8_t  data[0];
    } __attribute__((packed));
    
    // packet from IP6 layer
    int bottom(std::shared_ptr<Packet>& pckt);
    
    // set the downstream delegate
    inline void set_ip6_out(downstream del)
    {
      this->ip6_out = del;
    }
    
    inline const IP6::addr& local_ip()
    {
      return localIP;
    }
    
    // message types & codes
    static inline bool is_error(uint8_t type)
    {
      return type < 128;
    }
    static std::string code_string(uint8_t type, uint8_t code);
    
    // calculate checksum of any ICMP message
    static uint16_t checksum(std::shared_ptr<PacketICMP6>& pckt);
    
    // provide a handler for a @type of ICMPv6 message
    inline void listen(type_t type, handler_t func)
    {
      listeners[type] = func;
    }
    
    // transmit packet downstream
    int transmit(std::shared_ptr<PacketICMP6>& pckt);
    
  private:
    std::map<type_t, handler_t> listeners;
    // connection to IP6 layer
    downstream ip6_out;
    // this network stacks IPv6 address
    IP6::addr& localIP;
  };
  
  class PacketICMP6 : public PacketIP6
  {
  public:
    inline ICMPv6::header& header()
    {
      return *(ICMPv6::header*) this->payload();
    }
    inline const ICMPv6::header& header() const
    {
      return *(ICMPv6::header*) this->payload();
    }
    
    inline uint8_t type() const
    {
      return header().type;
    }
    inline uint8_t code() const
    {
      return header().code;
    }
    inline uint16_t checksum() const
    {
      return ntohs(header().checksum);
    }
 };
  
}
