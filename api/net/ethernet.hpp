#ifndef CLASS_ETHERNET_HPP
#define CLASS_ETHERNET_HPP

#include <net/inet_common.hpp>
#include <delegate>
#include <string>
//#include <net/class_packet.hpp>


namespace net {
  class Packet;
  
  /** Ethernet packet handling. */
  class Ethernet{
  
  public:
    static const int ETHER_ADDR_LEN = 6;
    
    union addr
    {
      uint8_t part[ETHER_ADDR_LEN];
      struct {
        uint16_t minor;
        uint32_t major;
      } __attribute__((packed));   
    
      inline std::string str() const {
        char _str[17];
        sprintf(_str,"%1x:%1x:%1x:%1x:%1x:%1x",
                part[0],part[1],part[2],
                part[3],part[4],part[5]);
        return std::string(_str);
      }
      
      inline bool operator == (addr& mac)
      { return strncmp((char*)mac.part,(char*)part,ETHER_ADDR_LEN) == 0; }
      
      inline addr& operator=(addr cpy){ 
	minor = cpy.minor;
	major = cpy.major;
	return *this;
      }
      
      static const addr MULTICAST_FRAME;
      static const addr BROADCAST_FRAME;
      static const addr IPv6mcast_01, IPv6mcast_02;
      
    } __attribute__((packed));
    
    struct header 
    {
      addr dest;
      addr src;
      unsigned short type;
    };
  
    /** Some big-endian ethertypes. 
        From http://en.wikipedia.org/wiki/EtherType. */
    enum ethertype_le {_ETH_IP4 = 0x0800, _ETH_ARP = 0x0806, 
                       _ETH_WOL = 0x0842, _ETH_IP6 = 0x86DD, 
                       _ETH_FLOW = 0x8808, _ETH_JUMBO = 0x8870};
  
    /** Little-endian ethertypes. */
    enum ethertype {ETH_IP4 = 0x8, ETH_ARP = 0x608, ETH_WOL = 0x4208,
                    ETH_IP6 = 0xdd86, ETH_FLOW = 0x888, ETH_JUMBO = 0x7088,
                    ETH_VLAN =0x81};
  
    /** Minimum payload */
    static constexpr int minimum_payload = 46;
  
    /** Bottom upstream input, "Bottom up". Handle raw ethernet buffer. */
    int bottom(Packet_ptr pckt);
  
    /** Delegate upstream ARP handler. 
	    @param delegate
	*/
    inline void set_arp_handler(upstream del)
    { _arp_handler = del; };
  
    /** Delegate upstream IPv4 handler.
        @param delegate	
	*/
    inline void set_ip4_handler(upstream del)
    { _ip4_handler = del; };
  
    /** Delegate upstream IPv6 handler. 
	    @param delegate
	*/
    inline void set_ip6_handler(upstream del)
    { _ip6_handler = del; };
  
    
    /** Delegate downstream 
	    @param delegate
	*/
    inline void set_physical_out(downstream del)
    { _physical_out = del; }
  
    inline const addr& mac()
    { return _mac; }

    /** Transmit data, with preallocated space for eth.header */
    int transmit(Packet_ptr pckt);
  
    Ethernet(addr mac);

  private:
    addr _mac;

    // Upstream OUTPUT connections
    upstream _ip4_handler;
    upstream _ip6_handler;
    upstream _arp_handler;
  
    // Downstream OUTPUT connection
    downstream _physical_out;
  
  
    /*
    
      +--|IP4|---|ARP|---|IP6|---+
      |                          |
      |        Ethernet          |
      |                          |
      +---------|Phys|-----------+
    
    */
  
};

} // ~net

#endif

