//-*- C++ -*-
#define DEBUG
#include <os>
#include <net/inet4.hpp>

namespace net
{
  
  template <typename T>  
  std::map<uint16_t, IP4::addr> Inet4<T>::_ip4_list;
  
  template <typename T>  
  std::map<uint16_t, IP4::addr> Inet4<T>::_netmask_list;
  
  template <typename T>  
  std::map<uint16_t, Ethernet*> Inet4<T>::_ethernet_list;    
  
  template <typename T> 
  std::map<uint16_t, Arp*> Inet4<T>::_arp_list;

  template <typename T>  
  void Inet4<T>::ifconfig(
      netdev i,
      IP4::addr ip,
      IP4::addr netmask )
  {
    _ip4_list[i]     = ip;
    _netmask_list[i] = netmask;
    
    //_ip6_list.insert(i);
    
    // register void* sp asm ("sp");
    // printf("ifconfig stack: %p\n", sp);
       
    debug("<Inet4> I now have %u IP's\n", _ip4_list.size());
  }

  template <typename T>  
  Inet4<T>::Inet4(Nic<T>& eth0) :
      //_eth(eth0.mac()),_arp(eth0.mac(),ip)
    _ip4(_ip4_list[0],_netmask_list[0]), _tcp(_ip4_list[0]), MTU(eth0.MTU()), bufstore_(eth0.bufstore())
  {
    debug("<IP Stack> Constructor. TCP @ %p has %i open ports. \n", &_tcp, _tcp.openPorts());
    
    
    
    /** Create arp- and ethernet objects for the interfaces.
        
        @warning: Careful not to copy these objects */
    _arp_list[0]      = new Arp(eth0.mac(),_ip4_list[0]);
    _ethernet_list[0] = new Ethernet(eth0.mac());
    
    Arp&      _arp = *_arp_list[0];
    Ethernet& _eth = *_ethernet_list[0];
    
    
    /** Upstream delegates */ 
    auto eth_bottom(upstream::from<Ethernet,&Ethernet::bottom>(_eth));
    auto arp_bottom(upstream::from<Arp,&Arp::bottom>(_arp));
    auto ip4_bottom(upstream::from<IP4,&IP4::bottom>(_ip4));
    auto icmp4_bottom(upstream::from<ICMP,&ICMP::bottom>(_icmp));
    auto udp4_bottom(upstream::from<UDP,&UDP::bottom>(_udp));
    auto tcp_bottom(upstream::from<TCP,&TCP::bottom>(_tcp));
    
    /** Upstream wiring  */
    
    // Phys -> Eth (Later, this will be passed through router)
    eth0.set_linklayer_out(eth_bottom);
    
    // Eth -> Arp
    _eth.set_arp_handler(arp_bottom);
    
    // Eth -> IP4
    _eth.set_ip4_handler(ip4_bottom);
    
    // IP4 -> ICMP
    _ip4.set_icmp_handler(icmp4_bottom);
    
    // IP4 -> UDP
    _ip4.set_udp_handler(udp4_bottom);
    
    // IP4 -> TCP
    _ip4.set_tcp_handler(tcp_bottom);
    
   
    /** Downstream delegates */
    auto phys_top(downstream
                  ::from<Nic<VirtioNet>,&Nic<VirtioNet>::transmit>(eth0));
    auto eth_top(downstream
                 ::from<Ethernet,&Ethernet::transmit>(_eth));    
    auto arp_top(downstream
                 ::from<Arp,&Arp::transmit>(_arp));
    auto ip4_top(downstream
                 ::from<IP4,&IP4::transmit>(_ip4));
    
    /** Downstream wiring. */
        
    // ICMP -> IP4
    _icmp.set_network_out(ip4_top);
    
    // UDP4 -> IP4
    _udp.set_network_out(ip4_top);
    
    // TCP -> IP4
    _tcp.set_network_out(ip4_top);


    // IP4 -> Arp    
    _ip4.set_linklayer_out(arp_top);
    
    // Arp -> Eth
    _arp.set_linklayer_out(eth_top);
    
    // Eth -> Phys
    _eth.set_physical_out(phys_top);
  }
}
