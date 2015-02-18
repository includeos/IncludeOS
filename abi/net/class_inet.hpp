
#ifndef CLASS_IP_STACK_HPP
#define CLASS_IP_STACK_HPP

#include <syscalls.hpp> // panic()
#include <class_dev.hpp> // 107: auto& eth0 = Dev::eth(0);
#include <net/class_ethernet.hpp>
#include <net/class_arp.hpp>
#include <net/class_ip4.hpp>
#include <net/class_ip6.hpp>
#include <net/class_icmp.hpp>
#include <net/class_udp.hpp>

#include <vector>

#include <class_nic.hpp>

namespace net {

  /** Nic names. Only used to bind nic to IP. */
  enum netdev {ETH0,ETH1,ETH2,ETH3,ETH4,ETH5,ETH6,ETH7,ETH8,ETH9};

  
class Inet {
  
public:
  
  /** Listen to a UDP port. 
      This is just a simple forwarder. @see UDP::listen.  */
  inline void udp_listen(uint16_t port, UDP::listener l)
  { _udp.listen(port,l); }

  /** Send a UDP datagram. 
      
      @note the data buffer is the *whole* ethernet frame, so don't overwrite 
      headers unless you own them (i.e. you *are* the IP object)  */
  inline int udp_send(std::shared_ptr<Packet> pckt)
  { return _udp.transmit(pckt); }
    
  
  /** Bind an IP and a netmask to a given device. 
      
      The function expects the given device to exist.*/
  static void ifconfig(netdev nic, IP4::addr ip, IP4::addr netmask);

  static inline IP4::addr ip4(netdev nic)
  { return _ip4_list[nic]; }
  
  static Inet* up(){
    if (_ip4_list.size() < 1)
      panic("<Inet> Can't bring up IP stack without any IP addresses");
    
    if (!instance){
      instance = new Inet();
      printf("<Inet> instance constructed @ 0x%lx\n",(uint32_t)instance);
    }
    return instance;
      
  };

  //typedef delegate<int(uint8_t*,int)> upstream_delg;

  
  
private:
  
  /** Physical routes. These map 1-1 with Dev:: interfaces. */
  static std::map<uint16_t,IP4::addr> _ip4_list;
  static std::map<uint16_t,IP4::addr> _netmask_list;
  static std::map<uint16_t,Ethernet*> _ethernet_list;
  static std::map<uint16_t,Arp*> _arp_list;
  
  static Inet* instance;  
  
  // This is the actual stack
  IP4 _ip4;
  IP6 _ip6;
  ICMP _icmp;
  UDP _udp;

  
  
  /** Don't think we *want* copy construction.
      @todo: Fix this with a singleton or something.
   */
  Inet(Inet& UNUSED(cpy)) :
    _ip4(_ip4_list[0],_netmask_list[0])
  {    
    printf("<IP Stack> WARNING: Copy-constructing the stack won't work."\
           "It should be pased by reference.\n");
    panic("Trying to copy-construct IP stack");
  }
  
  Inet(std::vector<IP4::addr> ips);

  /** Initialize. For now IP and mac is passed on to Ethernet and Arp.
      @todo For now, mac- and IP-addresses are hardcoded here. 
      They should be user-definable
   */
  Inet() :
    //_eth(eth0.mac()),_arp(eth0.mac(),ip)
    _ip4(_ip4_list[0],_netmask_list[0])
  {
    
    printf("<IP Stack> constructing \n");
    
    // For now we're just using the one interface
    auto& eth0 = Dev::eth(0);
    
    
    /** Create arp- and ethernet objects for the interfaces.
        
        @warning: Careful not to copy these objects */
    _arp_list[0] = new Arp(eth0.mac(),_ip4_list[0]);
    _ethernet_list[0] = new Ethernet(eth0.mac());
    
    Arp& _arp = *(_arp_list[0]);
    Ethernet& _eth = *(_ethernet_list[0]);


    
    
    /** Upstream delegates */ 
    auto eth_bottom(upstream::from<Ethernet,&Ethernet::bottom>(_eth));
    auto arp_bottom(upstream::from<Arp,&Arp::bottom>(_arp));
    auto ip4_bottom(upstream::from<IP4,&IP4::bottom>(_ip4));
    auto ip6_bottom(upstream::from<IP6,&IP6::bottom>(_ip6));    
    auto icmp_bottom(upstream::from<ICMP,&ICMP::bottom>(_icmp));
    auto udp_bottom(upstream::from<UDP,&UDP::bottom>(_udp));

    /** Upstream wiring  */
    
    // Phys -> Eth (Later, this will be passed through router)
    eth0.set_linklayer_out(eth_bottom);
    
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
  
};

}

#endif
