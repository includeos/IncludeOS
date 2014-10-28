#ifndef CLASS_IP_STACK_HPP
#define CLASS_IP_STACK_HPP


#include <net/class_ethernet.hpp>
#include <net/class_arp.hpp>
#include <net/class_ip4.hpp>
#include <net/class_ip6.hpp>
#include <net/class_icmp.hpp>
#include <net/class_udp.hpp>

namespace net {

  /** Nic names. Only used to bind nic to IP. */
  enum netdev {ETH0,ETH1,ETH2,ETH3,ETH4,ETH5,ETH6,ETH7,ETH8,ETH9};

  
class IP_stack {
  
  /** Physical layer output */
  delegate<int(uint8_t*,int)> _physical_out;
  
  // Ethernet and ARP will be removed from the stack and bound to interfaces
  Ethernet _eth;
  Arp _arp;
  
  // This is the actual stack
  IP4 _ip4;
  IP6 _ip6;
  ICMP _icmp;
  UDP _udp;
  
public:
  
  /** Physical layer input. */
  inline int physical_in(uint8_t* data,int len){
    return _eth.physical_in(data,len);
  };
  
  /** Delegate physical layer output. This completes the  downstream wiring.*/
  inline void set_physical_out(delegate<int(uint8_t*,int)> phys){
            
    // Eth -> Physical out
    _eth.set_physical_out(phys);
    
    // Maybe not necessary - for later rewiring
    _physical_out = phys;
  };
  
  /** Listen to a UDP port. 
      This is just a simple forwarder. @see UDP::listen.  */
  inline void udp_listen(uint16_t port, UDP::listener l)
  { _udp.listen(port,l); }

  /** Send a UDP datagram. 
      
      @note the data buffer is the *whole* ethernet frame, so don't overwrite 
      headers unless you own them (i.e. you *are* the IP object)  */
  inline int udp_send(IP4::addr sip,UDP::port sport,
                       IP4::addr dip,UDP::port dport,
                       uint8_t* data, int len)
  { return _udp.transmit(sip,sport,dip,dport,data,len); }

  
  inline IP4::addr& ip4() { return _arp.ip(); }
    
  
  /** Bind an IP and a netmask to a given device. 
      
      The function expects the given device to exist.*/
  int ifconfig(netdev nic, IP4::addr ip, IP4::addr netmask);
  
  
  
  /** Don't think we *want* copy construction.
      @todo: Fix this with a singleton or something.
   */
  IP_stack(IP_stack& cpy):
    _eth(cpy._eth),_arp(cpy._arp),_ip4(cpy._ip4)
  {    
    printf("<IP Stack> WARNING: Copy-constructing the stack won't work."\
           "It should be pased by reference.\n");
  }
  
  /** Initialize. For now IP and mac is passed on to Ethernet and Arp.
      @todo For now, mac- and IP-addresses are hardcoded here. 
      They should be user-definable
   */
  IP_stack(Ethernet::addr mac, IP4::addr ip) :
    _eth(mac),_arp(mac,ip)
  {
    
    printf("<IP Stack> constructing \n");
    
    /** Upstream delegates */ 
    auto arp_bottom(delegate<int(uint8_t*,int)>
                    ::from<Arp,&Arp::bottom>(_arp));
    auto ip4_bottom(delegate<int(uint8_t*,int)>
                    ::from<IP4,&IP4::bottom>(_ip4));
    auto ip6_bottom(delegate<int(uint8_t*,int)>
                    ::from<IP6,&IP6::bottom>(_ip6));    
    auto icmp_bottom(delegate<int(uint8_t*,int)>
                     ::from<ICMP,&ICMP::bottom>(_icmp));
    auto udp_bottom(delegate<int(uint8_t*,int)>
                    ::from<UDP,&UDP::bottom>(_udp));

    /** Upstream wiring  */
    
    // Eth -> Arp
    _eth.set_arp_handler(arp_bottom);
    
    // Eth -> IP4
    _eth.set_ip4_handler(ip4_bottom);
    
    // Eth -> IP6
    _eth.set_ip6_handler(ip6_bottom);
    
    // IP4 -> ICMP
    _ip4.set_icmp_handler(icmp_bottom);
    
    // IP4 -> UDP
    _ip4.set_udp_handler(udp_bottom);
    
    /** Downstream delegates */
    auto eth_top(delegate<int(Ethernet::addr,Ethernet::ethertype,uint8_t*,int)>
                 ::from<Ethernet,&Ethernet::transmit>(_eth));    
    auto arp_top(delegate<int(IP4::addr, IP4::addr, uint8_t*, uint32_t)>
                 ::from<Arp,&Arp::transmit>(_arp));
    auto ip4_top(delegate<int(IP4::addr,IP4::addr,IP4::proto,uint8_t*,uint32_t)>
                 ::from<IP4,&IP4::transmit>(_ip4));
    
    /** Downstream wiring. */
    
    // UDP -> IP4
    _udp.set_network_out(ip4_top);
    
    // ICMP -> IP4
    _icmp.set_network_out(ip4_top);
    
    // IP4 -> Arp    
    _ip4.set_linklayer_out(arp_top);
    
    // Arp -> Eth
    _arp.set_linklayer_out(eth_top);
    
    
    // Eth -> Physial out
    /** This can't be done until the physical interface is ready, so 
        downstream wiring must be completed with a call to "set_linklayer_out"*/
    
  }
  
};

}

#endif
