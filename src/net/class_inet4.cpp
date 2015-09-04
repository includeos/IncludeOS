#define DEBUG
#include <os>
#include <net/class_inet4.hpp>

namespace net
{
  Inet4* Inet4::instance = nullptr;
  
  std::map<uint16_t, IP4::addr> Inet4::_ip4_list;
  std::map<uint16_t, IP4::addr> Inet4::_netmask_list;
  std::map<uint16_t, Ethernet*> Inet4::_ethernet_list;
  std::map<uint16_t, Arp*> Inet4::_arp_list;
  
  void Inet4::ifconfig(
      netdev i,
      IP4::addr ip,
      IP4::addr netmask )
  {
    _ip4_list[i]     = ip;
    _netmask_list[i] = netmask;
    
    //_ip6_list.insert(i);
    
    register void* sp asm ("sp");
    printf("ifconfig stack: %p\n", sp);
       
    debug("<Inet4> I now have %lu IP's\n", _ip4_list.size());
  }

  Inet4::Inet4() :
      //_eth(eth0.mac()),_arp(eth0.mac(),ip)
      _ip4(_ip4_list[0],_netmask_list[0])
  {
    printf("<IP Stack> Constructor\n");
    
    // For now we're just using the one interface
    auto& eth0 = Dev::eth(0);
    
    
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
    
    // UDP -> IP4
    _udp.set_network_out(ip4_top);

    // IP4 -> Arp    
    _ip4.set_linklayer_out(arp_top);
    
    // Arp -> Eth
    _arp.set_linklayer_out(eth_top);
    
    // Eth -> Phys
    _eth.set_physical_out(phys_top);
  }
}
