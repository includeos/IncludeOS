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

#ifndef NET_ARP_HPP
#define NET_ARP_HPP

#include <delegate>
#include <net/ip4.hpp>
#include <net/inet.hpp>
#include <map>


namespace net {
  
  class PacketArp;
  
  /** Arp manager, including arp-cache. */
  class Arp {
     
  public:
    
    /** You can assign your own arp-resolution delegate. 
	We're doing this to keep the Hårek Haugerud mapping (HH_MAP) */
    using Arp_resolver =  delegate<int(Packet_ptr packet)>;

    enum Opcode { H_request = 0x100, H_reply = 0x200 };
    
    // Arp opcodes (Big-endian)
    static const uint16_t H_htype_eth = 0x0100;
    static const uint16_t H_ptype_ip4 = 0x0008;
    static const uint16_t H_hlen_plen = 0x0406;
    
    
    struct __attribute__((packed)) header {
      Ethernet::header ethhdr;   // Ethernet header
      uint16_t htype;            // Hardware type
      uint16_t ptype;            // Protocol type
      uint16_t hlen_plen;        // Protocol address length
      uint16_t opcode;           // Opcode
      Ethernet::addr shwaddr;    // Source mac
      IP4::addr sipaddr;         // Source ip
      Ethernet::addr dhwaddr;    // Target mac
      IP4::addr dipaddr;         // Target ip
    };

    /** Temporary type of protocol buffer. @todo encapsulate.*/
    typedef uint8_t* pbuf;

  
    /** Handle incoming ARP packet. */
    //int bottom(uint8_t* data, int len);
    void bottom(Packet_ptr pckt);

    /** Roll your own arp-resolution system. */
    void set_resolver(Arp_resolver ar){
      arp_resolver_ = ar;
    }
    
    
    enum Resolver_name { DEFAULT, HH_MAP };
    void set_resolver(Resolver_name nm){
      switch (nm) {
      case HH_MAP:	
	arp_resolver_ = Arp_resolver::from<Arp,&Arp::hh_map>(*this);
	break;
      default:
	Arp_resolver arp_resolver_ = Arp_resolver::from<Arp,&Arp::arp_resolve>(*this);	
      }
    }    
    
    /** Delegate link-layer output. */
    inline void set_linklayer_out(downstream link){
      linklayer_out_ = link;
    };

    /** Downstream transmission. */
    void transmit(Packet_ptr pckt);
    
    
    /** Get IP4 address */
    inline const IP4::addr& ip() { return ip_; }

    
    Arp(Inet<Ethernet,IP4>& inet);
  
  private: 
  
    Inet<Ethernet,IP4>& inet_;
    
    // Needs to know which mac address to put in header->swhaddr
    Ethernet::addr mac_;

    // Needs to know which IP to respond to
    const IP4::addr& ip_;
    
    // Outbound data goes through here
    downstream linklayer_out_;

    /** Cache entries are just macs and timestamps */
    struct cache_entry{
      Ethernet::addr mac_;
      uint64_t t_;
    
      cache_entry(){}; // map needs empty constructor (we have no emplace yet)
      cache_entry(Ethernet::addr mac) :mac_(mac),t_(OS::uptime()) {};
      cache_entry(const cache_entry& cpy)
      { mac_.major = cpy.mac_.major; mac_.minor = cpy.mac_.minor; t_ = cpy.t_; }
      void update() { t_ = OS::uptime(); }
    };
  
    // The arp cache
    std::map<IP4::addr,cache_entry> cache_;
  
    // Arp cache expires after cache_exp_t seconds
    static constexpr uint16_t cache_exp_t_ = 60 * 60 * 12;

    /** Cache IP resolution. */
    void cache(IP4::addr&, Ethernet::addr&);
  
    /** Checks if an IP is cached and not expired */
    bool is_valid_cached(IP4::addr&);

    /** Arp resolution. */
    Ethernet::addr& resolve(IP4::addr&);
  
    
    void arp_respond(header* hdr_in);    
        
    int hh_map(Packet_ptr packet);
    int arp_resolve(Packet_ptr packet);
    Arp_resolver arp_resolver_ = Arp_resolver::from<Arp,&Arp::arp_resolve>(*this);
    
    std::map<IP4::addr, Packet_ptr> waiting_packets_;
    
    /** Add a packet to waiting queue, to be sent when IP is resolved */
    void await_resolution(Packet_ptr, IP4::addr);
    
    /** Create a default initialized ARP-packet */
    Packet_ptr createPacket();
    
  };

} // ~net
#endif
