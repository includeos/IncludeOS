//-*- C++ -*-
#define DEBUG
#include <os>
#include <net/inet4.hpp>

namespace net
{
  
  template <typename T>  
  Inet4<T>::Inet4(Nic<T>& nic, IP4::addr ip, IP4::addr netmask) :
    ip4_addr_(ip), netmask_(netmask), nic_(nic), eth_(nic.mac()), arp_(*this), 
    ip4_(*this), icmp_(*this), tcp_(*this), udp_(*this), MTU(nic.MTU()), bufstore_(nic.bufstore())
  {
    debug("<IP Stack> Constructor. TCP @ %p has %i open ports. \n", &_tcp, _tcp.openPorts());          
    
    /** Upstream delegates */ 
    auto eth_bottom(upstream::from<Ethernet,&Ethernet::bottom>(eth_));
    auto arp_bottom(upstream::from<Arp,&Arp::bottom>(arp_));
    auto ip4_bottom(upstream::from<IP4,&IP4::bottom>(ip4_));
    auto icmp4_bottom(upstream::from<ICMP,&ICMP::bottom>(icmp_));
    auto udp4_bottom(upstream::from<UDP,&UDP::bottom>(udp_));
    auto tcp_bottom(upstream::from<TCP,&TCP::bottom>(tcp_));
    
    /** Upstream wiring  */
    
    // Phys -> Eth (Later, this will be passed through router)
    nic.set_linklayer_out(eth_bottom);
    
    // Eth -> Arp
    eth_.set_arp_handler(arp_bottom);
    
    // Eth -> IP4
    eth_.set_ip4_handler(ip4_bottom);
    
    // IP4 -> ICMP
    ip4_.set_icmp_handler(icmp4_bottom);
    
    // IP4 -> UDP
    ip4_.set_udp_handler(udp4_bottom);
    
    // IP4 -> TCP
    ip4_.set_tcp_handler(tcp_bottom);
    
   
    /** Downstream delegates */
    auto phys_top(downstream
                  ::from<Nic<VirtioNet>,&Nic<VirtioNet>::transmit>(nic));
    auto eth_top(downstream
                 ::from<Ethernet,&Ethernet::transmit>(eth_));    
    auto arp_top(downstream
                 ::from<Arp,&Arp::transmit>(arp_));
    auto ip4_top(downstream
                 ::from<IP4,&IP4::transmit>(ip4_));
    
    /** Downstream wiring. */
        
    // ICMP -> IP4
    icmp_.set_network_out(ip4_top);
    
    // UDP4 -> IP4
    udp_.set_network_out(ip4_top);
    
    // TCP -> IP4
    tcp_.set_network_out(ip4_top);

    // IP4 -> Arp    
    ip4_.set_linklayer_out(arp_top);
    
    // Arp -> Eth
    arp_.set_linklayer_out(eth_top);
    
    // Eth -> Phys
    eth_.set_physical_out(phys_top);
  }
}
