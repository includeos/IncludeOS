#ifndef CLASS_IP6_HPP
#define CLASS_IP6_HPP

#include <delegate>
#include <net/inet.hpp>

#include <net/class_ethernet.hpp>

#include <iostream>
#include <string>
#include <x86intrin.h>

namespace net
{
  class Packet;
  
  /** IP6 layer skeleton */
  class IP6
  {
  public:
    /** Known transport layer protocols. */
    enum proto{PROTO_ICMP=1, PROTO_UDP=17, PROTO_TCP=6};
    
    /** Handle IPv6 packet. */
    int bottom(std::shared_ptr<Packet>& pckt);
    
    class addr
    {
    public:
      addr(unsigned a, unsigned b, unsigned c, unsigned d)
        : i32{a, b, c, d} {}
      addr(const addr& a) : i128(a.i128) {}
      
      union
      {
        __m128i  i128;
        uint64_t i64[2];
        uint32_t i32[4];
        uint16_t i16[8];
        uint8_t  i8[16];
      };
      
      std::string to_string() const;
    };
    
    #pragma pack(push, 1)
    class header
    {
    public:
      uint8_t getVersion() const
      {
        return (scanline[0] & 0xF0) >> 4;
      }
      uint8_t getClass() const
      {
        return ((scanline[0] & 0xF000) >> 12) + (scanline[0] & 0xF);
      }
      
      // 128-bit is probably not good as "by value"
      const addr& getSource() const
      {
        return source;
      }
      const addr& getDest() const
      {
        return dest;
      }
      
    //private:
      uint32_t scanline[2];
      addr     source;
      addr     dest;
    };
    #pragma pack(pop)
    
    struct full_header
    {
      Ethernet::header eth_hdr;
      IP6::header      ip6_hdr;
    };
    
    /** Constructor. Requires ethernet to latch on to. */
    IP6(const addr& local);
    
    const IP6::addr& getIP() const
    {
      return local;
    }
    
  private:
    addr local;
    
  };
  
} // namespace net

#endif
