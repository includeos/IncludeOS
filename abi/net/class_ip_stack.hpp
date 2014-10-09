#ifndef CLASS_IP_STACK_HPP
#define CLASS_IP_STACK_HPP


#include <net/class_ethernet.hpp>
#include <net/class_arp.hpp>
#include <net/class_ip4.hpp>
#include <net/class_ip6.hpp>

class IP_stack {
  
  /** Physical layer output */
  delegate<int(uint8_t*,int)> _physical_out;
  
  Ethernet _eth;
  Arp _arp;
  IP4 _ip4;
  IP6 _ip6;
  
public:
  /** Physical layer input. */
  inline int physical_in(uint8_t* data,int len){
    return _eth.physical_in(data,len);
  };
  
  /** Delegate physical layer output. */
  inline void physical_out(delegate<int(uint8_t*,int)> phys){
    _eth.set_physical_out(phys);
  };
  
  IP_stack() :
    _arp(0xb00a8c0)
  {
    // Make delegates for bottom of layers
    auto arp_bottom(delegate<int(uint8_t*,int)>::from<Arp,&Arp::bottom>(_arp));
    auto ip4_bottom(delegate<int(uint8_t*,int)>::from<IP4,&IP4::bottom>(_ip4));
    auto ip6_bottom(delegate<int(uint8_t*,int)>::from<IP6,&IP6::bottom>(_ip6));    
    
    // Hook up layers on top of ethernet
    // Upstream:
    _eth.set_arp_handler(arp_bottom);
    _eth.set_ip4_handler(ip4_bottom);
    _eth.set_ip6_handler(ip6_bottom);
    
    
  }
  
};


#endif
