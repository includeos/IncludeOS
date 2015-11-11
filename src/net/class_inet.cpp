#define DEBUG
#include <os>
#include <net/class_inet.hpp>

namespace net
{
  
  std::map<uint16_t, IP4::addr> Inet::_ip4_list;
  std::map<uint16_t, IP4::addr> Inet::_netmask_list;
  std::map<uint16_t, IP6::addr> Inet::_ip6_list;
  std::map<uint16_t, Ethernet*> Inet::_ethernet_list;
  std::map<uint16_t, Arp*> Inet::_arp_list;
  
  void Inet::ifconfig(
      netdev i,
      IP4::addr ip,
      IP4::addr netmask,
      IP6::addr ip6)
  {
    _ip4_list[i]     = ip;
    _netmask_list[i] = netmask;
    _ip6_list[i]     = ip6;
    //debug("ifconfig ipv6 addr: %s\n", _ip6_list[i].to_string().c_str());
    
    debug("<Inet> I now have %lu IPv4's\n", _ip4_list.size());
    debug("<Inet> I now have %lu IPv6's\n", _ip6_list.size());
  }

  Inet::Inet() :
      //_eth(eth0.mac()),_arp(eth0.mac(),ip)
      _ip4(_ip4_list[0],_netmask_list[0]),
      _ip6(_ip6_list[0]),
      _icmp6(_ip6_list[0]), _udp6(_ip6_list[0])
  {
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
    
    auto ip6_bottom  (upstream::from<IP6,   &IP6::bottom>   (_ip6));    
    auto icmp6_bottom(upstream::from<ICMPv6,&ICMPv6::bottom>(_icmp6));
    auto udp6_bottom (upstream::from<UDPv6, &UDPv6::bottom> (_udp6));
    
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
    
    // Ethernet -> IP6
    _eth.set_ip6_handler(ip6_bottom);
    // IP6 packet transmission
    auto ip6_transmit(IP6::downstream6::from<IP6,&IP6::transmit>(_ip6));
    // IP6 -> ICMP6
    _ip6.set_handler(IP6::PROTO_ICMPv6, icmp6_bottom);
    // IP6 <- ICMP6
    _icmp6.set_ip6_out(ip6_transmit);
    // IP6 -> UDP6
    _ip6.set_handler(IP6::PROTO_UDP, udp6_bottom);
    // IP6 <- UDP6
    _udp6.set_ip6_out(ip6_transmit);
    
    // IP6 -> Ethernet
    auto ip6_to_eth(downstream::from<Ethernet, &Ethernet::transmit>(_eth));
    _ip6.set_linklayer_out(ip6_to_eth);
    
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
