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

#ifndef CLASS_IP4_HPP
#define CLASS_IP4_HPP

#include <delegate>

#include <net/inet_common.hpp>
#include <net/ethernet.hpp>
#include <net/inet.hpp>
#include <iostream>
#include <string>

namespace net {
  
  class Packet;
  
  // Default delegate assignments
  int ignore_ip4_up(Packet_ptr);
  int ignore_ip4_down(Packet_ptr);
  
  /** IP4 layer */
  class IP4 {
  public:
    
    /** Known transport layer protocols. */
    enum proto{IP4_ICMP=1, IP4_UDP=17, IP4_TCP=6};
    
    /** IP4 address */
    union __attribute__((packed)) addr{
      uint8_t part[4];
      uint32_t whole;
      
      // Constructors:
      // Can't have them - that removes the packed-attribute    
      inline addr& operator=(addr cpy){
        whole = cpy.whole;
        return *this;
      }
      
      // Standard operators 
      inline bool operator==(addr rhs) const
      { return whole == rhs.whole; }
      
      inline bool operator<(const addr rhs) const
      { return whole < rhs.whole; }

      inline bool operator>(const addr rhs) const
      { return ! (*this < rhs); }
      
      inline bool operator!=(const addr rhs) const
      { return whole != rhs.whole; }
      
      inline bool operator!=(const uint32_t rhs) const
      { return  whole != rhs; }
      
      std::string str() const
      {
        char ip_addr[16];
        sprintf(ip_addr, "%1i.%1i.%1i.%1i",
                part[0], part[1], part[2], part[3]);
        return ip_addr;
      }      
    };
    
    static const addr INADDR_ANY;
    static const addr INADDR_BCAST;
    
    /** IP4 header */
    struct ip_header{
      uint8_t version_ihl;
      uint8_t tos;
      uint16_t tot_len;
      uint16_t id;
      uint16_t frag_off_flags;
      uint8_t ttl;
      uint8_t protocol;
      uint16_t check;
      addr saddr;
      addr daddr;
    };

    /** The full header including IP. 
     *  @Note : This might be removed if we decide to isolate layers more.
     */
    struct full_header{
      uint8_t link_hdr [sizeof(typename LinkLayer::header)];
      ip_header ip_hdr;
    };
        
    /** Upstream: Input from link layer. */
    int bottom(Packet_ptr pckt);
    
    /** Upstream: Outputs to transport layer*/
    inline void set_icmp_handler(upstream s)
    { icmp_handler_ = s; }
    
    inline void set_udp_handler(upstream s)
    { udp_handler_ = s; }
    
    inline void set_tcp_handler(upstream s)
    { tcp_handler_ = s; }
    
    /** Downstream: Delegate linklayer out */
    void set_linklayer_out(downstream s)
    { linklayer_out_ = s; };
  
    /** Downstream: Receive data from above and transmit. 
        
        @note The following *must be set* in the packet:
        
        * Destination IP
        * Protocol
        
        Source IP *can* be set - if it's not, IP4 will set it.
        
    */
    int transmit(Packet_ptr pckt);

    /** Compute the IP4 header checksum */
    uint16_t checksum(ip_header* hdr);
    
    /**
     * \brief
     * Returns the IPv4 address associated with this interface
     * 
     **/
    const addr& local_ip() const
    {
      return stack.ip_addr();
    }
    
    /** Initialize. Sets a dummy linklayer out. */
    explicit IP4(Inet<LinkLayer,IP4>&);
  
  private:
    Inet<LinkLayer,IP4>& stack;
    
    /** Downstream: Linklayer output delegate */
    downstream linklayer_out_ = downstream(ignore_ip4_down);;
    
    /** Upstream delegates */
    upstream icmp_handler_ = upstream(ignore_ip4_up);
    upstream udp_handler_ = upstream(ignore_ip4_up);
    upstream tcp_handler_ = upstream(ignore_ip4_up);
  };
  
} // ~net
#endif