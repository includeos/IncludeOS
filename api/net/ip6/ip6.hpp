// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef NET_IP6_IP6_HPP
#define NET_IP6_IP6_HPP

#include <delegate>
#include "../ethernet.hpp"
#include "../packet.hpp"
#include "../util.hpp"

#include <debug>

#include <iostream>
#include <string>
#include <map>
#include <x86intrin.h>

#include <stdint.h>
#include <assert.h>

namespace net
{
  class PacketIP6;
  
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
    
    struct addr
    {
      // constructors
      addr()
        : i32{0, 0, 0, 0} {}
      addr(uint16_t a1, uint16_t a2, uint16_t b1, uint16_t b2, 
           uint16_t c1, uint16_t c2, uint16_t d1, uint16_t d2)
      {
        i16[0] = a1; i16[1] = a2;
        i16[2] = b1; i16[3] = b2;
        i16[4] = c1; i16[5] = c2;
        i16[6] = d1; i16[7] = d2;
        /*i128 = _mm_set_epi16(
           htons(d2), htons(d1), 
           htons(c2), htons(c1), 
           htons(b2), htons(b1), 
           htons(a2), htons(a1));*/
      }
      addr(uint32_t a, uint32_t b, uint32_t c, uint32_t d)
      {
        i32[0] = a; i32[1] = b; i32[2] = c; i32[3] = d;
        //i128 = _mm_set_epi32(d, c, b, a);
      }
      addr(const addr& a)
      {
        for (int i = 0; i < 4; i++)
          i32[i] = a.i32[i];
      }
      // move constructor
      addr& operator= (const addr& a)
      {
        for (int i = 0; i < 4; i++)
          i32[i] = a.i32[i];
        //i128 = a.i128;
        return *this;
      }
      
      // comparison functions
      bool operator== (const addr& a) const
      {
        // i128 == a.i128:
        for (int i = 0; i < 4; i++)
          if (i32[i] != a.i32[i]) return false;
        return true;
        //__m128i cmp = _mm_cmpeq_epi32(i128, a.i128);
        //return _mm_cvtsi128_si32(cmp);
      }
      bool operator!= (const addr& a) const
      {
        return !this->operator==(a);
      }
      
      // returns this IPv6 address as a string
      std::string str() const;
      
      // multicast IPv6 addresses
      static const addr node_all_nodes;     // RFC 4921
      static const addr node_all_routers;   // RFC 4921
      static const addr node_mDNSv6;        // RFC 6762 (multicast DNSv6)
      
      // unspecified link-local address
      static const addr link_unspecified;
      
      // RFC 4291  2.4.6:
      // Link-Local addresses are designed to be used for addressing on a
      // single link for purposes such as automatic address configuration,
      // neighbor discovery, or when no routers are present.
      static const addr link_all_nodes;     // RFC 4921
      static const addr link_all_routers;   // RFC 4921
      static const addr link_mDNSv6;        // RFC 6762
      
      static const addr link_dhcp_servers;  // RFC 3315
      static const addr site_dhcp_servers;  // RFC 3315
      
      // returns true if this addr is a IPv6 multicast address
      bool is_multicast() const
      {
        /**
           RFC 4291 2.7 Multicast Addresses
          
           An IPv6 multicast address is an identifier for a group of interfaces
           (typically on different nodes). An interface may belong to any
           number of multicast groups. Multicast addresses have the following format:
           |   8    |  4 |  4 |                  112 bits                   |
           +------ -+----+----+---------------------------------------------+
           |11111111|flgs|scop|                  group ID                   |
           +--------+----+----+---------------------------------------------+
        **/
        return i8[0] == 0xFF;
      }
      
      union
      {
        //__m128i  i128;
        uint32_t  i32[ 4];
        uint16_t  i16[ 8];
        uint8_t    i8[16];
      };
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
      // initializes the first scanline with the IPv6 version
      void init_scan0()
      {
        scanline[0] = 6u >> 4;
      }
      
      uint16_t size() const
      {
        return ((scanline[1] & 0x00FF) << 8) +
          ((scanline[1] & 0xFF00) >> 8);
      }
      void set_size(uint16_t newsize)
      {
        scanline[1] &= 0xFFFF0000;
        scanline[1] |= htons(newsize);
      }
      
      uint8_t next() const
      {
        return (scanline[1] >> 16) & 0xFF;
      }
      void set_next(uint8_t next)
      {
        scanline[1] &= 0xFF00FFFF;
        scanline[1] |= next << 16;
      }
      uint8_t hoplimit() const
      {
        return (scanline[1] >> 24) & 0xFF;
      }
      void set_hoplimit(uint8_t limit = 64)
      {
        scanline[1] &= 0x00FFFFFF;
        scanline[1] |= limit << 24;
      }
      
    private:
      uint32_t scanline[2];
    public:
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
#pragma pack(pop)
    
    struct full_header
    {
      Ethernet::header eth_hdr;
      IP6::header      ip6_hdr;
    };
    
    // downstream delegate for transmit()
    typedef delegate<int(std::shared_ptr<PacketIP6>&)> downstream6;
    typedef downstream6 upstream6;
    
    /** Constructor. Requires ethernet to latch on to. */
    IP6(const addr& local);
    
    const IP6::addr& local_ip() const
    {
      return local;
    }
    
    uint8_t parse6(uint8_t*& reader, uint8_t next);
    
    static std::string protocol_name(uint8_t protocol)
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
    
    // handler for upstream IPv6 packets
    void bottom(Packet_ptr pckt);
    
    // transmit packets to the ether
    void transmit(std::shared_ptr<PacketIP6>& pckt);
    
    // modify upstream handlers
    inline void set_handler(uint8_t proto, upstream& handler)
    {
      proto_handlers[proto] = handler;
    }
    
    inline void set_linklayer_out(downstream func)
    {
      _linklayer_out = func;
    }
    
    // creates a new IPv6 packet to be sent over the ether
    static std::shared_ptr<PacketIP6> create(uint8_t proto,
                                             Ethernet::addr ether_dest, const IP6::addr& dest);
    
  private:
    addr local;
    
    /** Downstream: Linklayer output delegate */
    downstream _linklayer_out;
    
    /** Upstream delegates */
    std::map<uint8_t, upstream> proto_handlers;
  };
  
  inline std::ostream& operator<< (std::ostream& out, const IP6::addr& ip)
  {
    return out << ip.str();
  }
  
} // namespace net

#endif
