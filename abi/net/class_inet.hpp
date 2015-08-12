#ifndef NET_INET_HPP
#define NET_INET_HPP

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
    static void
    ifconfig(
        netdev nic,
        IP4::addr ip,
        IP4::addr netmask,
        IP6::addr ip6);
    
    inline static IP4::addr ip4(netdev nic)
    { return _ip4_list[nic]; }
    
    inline static IP6::addr ip6(netdev nic)
    { return _ip6_list[nic]; }
    
    static Inet* up(){
      if (_ip4_list.size() < 1)
	panic("<Inet> Can't bring up IP stack without any IP addresses");
      
      if (!instance){
	instance = new Inet();
	printf("<Inet> instance constructed @ %p\n", instance);
      }
      return instance;
      
    };
    
    //typedef delegate<int(uint8_t*,int)> upstream_delg;
    
    
  
  private:
    
    /** Physical routes. These map 1-1 with Dev:: interfaces. */
    static std::map<uint16_t,IP4::addr> _ip4_list;
    static std::map<uint16_t,IP4::addr> _netmask_list;
    static std::map<uint16_t,IP6::addr> _ip6_list;
    static std::map<uint16_t,Ethernet*> _ethernet_list;
    static std::map<uint16_t,Arp*> _arp_list;
    
    static Inet* instance;  
    
    // This is the actual stack
    IP4  _ip4;
    ICMP _icmp;
    UDP  _udp;
    IP6    _ip6;
    ICMPv6 _icmp6;
    UDPv6  _udp6;
    
    
    /** Don't think we *want* copy construction.
	@todo: Fix this with a singleton or something.
   */
    Inet(Inet& UNUSED(cpy)) :
      _ip4(_ip4_list[0],_netmask_list[0]),
      _ip6(ip6(ETH0))
    {    
      printf("<IP Stack> WARNING: Copy-constructing the stack won't work." \
	     "It should be pased by reference.\n");
      panic("Trying to copy-construct IP stack");
    }
    
    Inet(std::vector<IP4::addr> ips);
    
    /** Initialize. For now IP and mac is passed on to Ethernet and Arp.
	@todo For now, mac- and IP-addresses are hardcoded here. 
	They should be user-definable
    */
    Inet();
    
  };

}

#endif
