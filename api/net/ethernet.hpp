// Part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
  
    // Minimum payload
    static constexpr int minimum_payload = 46;
  
    /** Bottom upstream input, "Bottom up". Handle raw ethernet buffer. */
    int bottom(Packet_ptr pckt);
  
    /** Delegate upstream ARP handler. */
    inline void set_arp_handler(upstream del)
    { arp_handler_ = del; }

    inline upstream get_arp_handler()
    { return arp_handler_; }
        
    /** Delegate upstream IPv4 handler. */
    inline void set_ip4_handler(upstream del)
    { ip4_handler_ = del; }
    
    /** Delegate upstream IPv4 handler. */
    inline upstream get_ip4_handler()
    { return ip4_handler_; }
  

    /** Delegate upstream IPv6 handler. */
    inline void set_ip6_handler(upstream del)
    { ip6_handler_ = del; };  
    
    /** Delegate downstream */
    inline void set_physical_out(downstream del)
    { physical_out_ = del; }
    
    /** Assign a packet filter. 
	@note : The filter function will be called as early as possible., 
	i.e. in bottom */
    inline void set_packet_filter(Packet_filter f)
    { filter_ = f; }
    
    /** @return Mac address of the underlying device */
    inline const addr& mac()
    { return _mac; }

    /** Transmit data, with preallocated space for eth.header */
    int transmit(Packet_ptr pckt);
  
    Ethernet(addr mac);

  private:
    addr _mac;

    // Upstream OUTPUT connections
    upstream ip4_handler_ = [](Packet_ptr)->int{ return 0; };
    upstream ip6_handler_ = [](Packet_ptr)->int{ return 0; };
    upstream arp_handler_ = [](Packet_ptr)->int{ return 0; };
    Packet_filter filter_ = [](Packet_ptr p)->Packet_ptr{ return p; };
    
    // Downstream OUTPUT connection
    downstream physical_out_ = [](Packet_ptr)->int{ return 0; };
  
  
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

