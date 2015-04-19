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
    enum proto
    {
      PROTO_HOPOPT =  0, // IPv6 hop-by-hop
      
      PROTO_ICMPv4 =  1,
      PROTO_TCP    =  6,
      PROTO_UDP    = 17,
      
      PROTO_ICMPv6 = 58, // IPv6 ICMP
      PROTO_NoNext = 59, // no next-header
      PROTO_OPTSv6 = 60, // dest options
    };
    
    /** Handle IPv6 packet. */
    int bottom(std::shared_ptr<Packet>& pckt);
    
    class addr
    {
    public:
      // constructors
      addr(unsigned a, unsigned b, unsigned c, unsigned d)
        : i32{a, b, c, d} {}
      addr(uint64_t top, uint64_t bot)
        : i64{top, bot} {}
      addr(__m128i address)
        : i128(address) {}
      // copy-constructor
      addr(const addr& a)
        : i128(a.i128) {}
      
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
      uint8_t version() const
      {
        return (scanline[0] & 0xF0) >> 4;
      }
      uint8_t tclass() const
      {
        return ((scanline[0] & 0xF000) >> 12) + 
                (scanline[0] & 0xF);
      }
      
      uint16_t size() const
      {
        return ((scanline[1] & 0x00FF) << 8) +
               ((scanline[1] & 0xFF00) >> 8);
      }
      
      uint8_t next() const
      {
        return (scanline[1] >> 16) & 0xFF;
      }
      uint8_t hoplimit() const
      {
        return (scanline[1] >> 24) & 0xFF;
      }
      
      // 128-bit is probably not good as "by value"
      const addr& source() const
      {
        return src;
      }
      const addr& dest() const
      {
        return dst;
      }
      
    private:
      uint32_t scanline[2];
      addr     src;
      addr     dst;
    };
    
    struct options_header
    {
      uint8_t  next_header;
      uint8_t  hdr_ext_len;
      uint16_t opt_1;
      uint32_t opt_2;
      
      uint8_t next() const
      {
        return next_header;
      }
      uint8_t size() const
      {
        return sizeof(options_header) + hdr_ext_len;
      }
      uint8_t extended() const
      {
        return hdr_ext_len;
      }
    };
    
    struct icmp_header
    {
      uint8_t  type_;
      uint8_t  code_;
      uint16_t checksum_;
      
      uint8_t type() const
      {
        return type_;
      }
      uint8_t code() const
      {
        return code_;
      }
      uint16_t checksum() const
      {
        return __builtin_bswap16( checksum_ );
      }
      
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
    
    uint8_t parse6(uint8_t*& reader, uint8_t next);
    
    std::string protocol_name(uint8_t protocol)
    {
      switch (protocol)
      {
      case PROTO_HOPOPT:
        return "IPv6 Hop-By-Hop (0)";
        
      case PROTO_TCP:
        return "TCPv6 (6)";
      case PROTO_UDP:
        return "UDPv6 (17)";
        
      case PROTO_ICMPv6:
        return "ICMPv6 (58)";
      case PROTO_NoNext:
        return "No next header (59)";
      case PROTO_OPTSv6:
        return "IPv6 destination options (60)";
        
      default:
        return "Unknown: " + std::to_string(protocol);
      }
    }
    
  private:
    addr local;
    
    /** Downstream: Linklayer output delegate */
    downstream _linklayer_out;
    
    /** Upstream delegates */
    upstream icmp_handler;
    upstream udp_handler;
    upstream tcp_handler;
  };
  
  inline std::ostream& operator<< (std::ostream& out, const IP6::addr& ip)
  {
    return out << ip.to_string();
  }
  
} // namespace net

#endif
