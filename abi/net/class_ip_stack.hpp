#ifndef CLASS_IP_STACK_HPP
#define CLASS_IP_STACK_HPP


#include <net/class_ethernet.hpp>
#include <net/class_arp.hpp>
#include <net/class_ip4.hpp>
#include <net/class_ip6.hpp>
#include <net/class_icmp.hpp>
#include <net/class_udp.hpp>

class IP_stack {
  
  /** Physical layer output */
  delegate<int(uint8_t*,int)> _physical_out;
  
  Ethernet _eth;
  Arp _arp;
  IP4 _ip4;
  IP6 _ip6;
  ICMP _icmp;
  UDP _udp;
  
public:
  
  /** Physical layer input. */
  inline int physical_in(uint8_t* data,int len){
    return _eth.physical_in(data,len);
  };
  
  /** Delegate physical layer output. */
  inline void set_physical_out(delegate<int(uint8_t*,int)> phys){
    _eth.set_physical_out(phys);
    
    // Temp: Routing arp directly to physical.
    _arp.set_linklayer_out(phys);
    
    // Maybe not necessary
    _physical_out = phys;
  };
  
  IP_stack() :
    _arp(IP4::addr({192,168,0,11}),
         Ethernet::addr({0x08,0x00,0x27,0x9D,0x86,0xE8}))
  {
    /** Make delegates for bottom of layers */
    
    auto arp_bottom(delegate<int(uint8_t*,int)>::from<Arp,&Arp::bottom>(_arp));
    auto ip4_bottom(delegate<int(uint8_t*,int)>::from<IP4,&IP4::bottom>(_ip4));
    auto ip6_bottom(delegate<int(uint8_t*,int)>::from<IP6,&IP6::bottom>(_ip6));
    

    auto icmp_bottom(delegate<int(uint8_t*,int)>::from<ICMP,&ICMP::bottom>(_icmp));
    auto udp_bottom(delegate<int(uint8_t*,int)>::from<UDP,&UDP::bottom>(_udp));

    // Hook up layers on top of ethernet
    // Upstream:
    _eth.set_arp_handler(arp_bottom);
    _eth.set_ip4_handler(ip4_bottom);
    _eth.set_ip6_handler(ip6_bottom);
    
    _ip4.set_icmp_handler(icmp_bottom);
    _ip4.set_udp_handler(udp_bottom);

    // Downstream:
    // Handled in set_physical_out. Everybody has to provide their own default
    // default downstream "gutter", i.e. a delegate to a function doing nothing.
    
  }
  
};


#endif
