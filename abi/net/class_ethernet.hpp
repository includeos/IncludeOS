#ifndef CLASS_ETHERNET_HPP
#define CLASS_ETHERNET_HPP

#include <delegate>

class Ethernet{
  
public:
  
#define ETHER_ADDR_LEN 6
  
  struct addr {
    uint8_t addr[ETHER_ADDR_LEN];
  };
  
  struct header 
  {
    struct addr dest;
    struct addr src;
    unsigned short type;
  };
  
  /** Some little-endian ethertypes. 
      From http://en.wikipedia.org/wiki/EtherType. */
  enum ethertype_le {_ETH_IP4 = 0x0800, _ETH_ARP = 0x0806, _ETH_WOL = 0x0842, 
                     _ETH_IP6 = 0x86DD, _ETH_FLOW = 0x8808, _ETH_JUMBO = 0x8870};
  
  /** Big-endian ethertypes. */
  enum ethertype {ETH_IP4 = 0x8, ETH_ARP = 0x608, ETH_WOL = 0x4208,
                  ETH_IP6 = 0xdd86, ETH_FLOW = 0x888, ETH_JUMBO = 0x7088};

  delegate<void(uint8_t* data, int len)> _ip4_handler;
  delegate<void(uint8_t* data, int len)> _ip6_handler;
  delegate<void(uint8_t* data, int len)> _arp_handler;
  
  /** Handle raw ethernet buffer. */
  void handler(uint8_t* data, int len);
    
  /** Set ARP handler. */
  inline void set_arp_handler(delegate<void(uint8_t* data, int len)> del)
  { _arp_handler = del; };
  
  /** Set IPv4 handler. */
  inline void set_ip4_handler(delegate<void(uint8_t* data, int len)> del)
  { _ip4_handler = del; };
  
  /** Set IPv6 handler. */
  inline void set_ip6_handler(delegate<void(uint8_t* data, int len)> del)
  { _ip6_handler = del; };
  
  
  Ethernet();
  
};

#endif

