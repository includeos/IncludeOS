#ifndef CLASS_IP4_HPP
#define CLASS_IP4_HPP

#include <delegate>
#include <net/class_ethernet.hpp>
#include <net/inet.hpp>

#include <iostream>
#include <string>

namespace net {

  /** IP4 layer skeleton */
  class IP4 {  
  public:
  
    /** Known transport layer protocols. */
    enum proto{IP4_ICMP=1, IP4_UDP=17, IP4_TCP=6};
  
    /** Signature for output-delegates. */
    typedef delegate<int(uint8_t* data,int len)> subscriber;  
  
    /** Temporary protocol buffer. Might encapsulate later.*/
    typedef uint8_t* pbuf;
  
    /** IP4 address */
    union __attribute__((packed)) addr{
      uint8_t part[4];
      uint32_t whole;
    
      // Constructors:
      // Can't have them - that removes the packed-attribute
    
      inline bool operator==(addr& src) const
      { return src.whole == whole; }
    
      inline bool operator<(const addr src) const
      { return src.whole < whole; }

      inline bool operator!=(const addr src) const
      { return src.whole != whole; }
    
      std::string str() const {
        char _str[15];
        sprintf(_str,"%1i.%1i.%1i.%1i",part[0],part[1],part[2],part[3]);
        return std::string(_str);
      }        
    
    };
  
    /** Delegate type for linklayer out. */
    typedef delegate<int(addr,addr,uint8_t* data,int len)> link_out;      
  
    /** Delegate type for input from upper layers. */
    typedef delegate<int(IP4::addr sip,IP4::addr dip, IP4::proto, 
                         pbuf buf, uint32_t len)> transmitter;
  

    /** IP4 header */
    struct ip_header{
      uint8_t version_ihl;
      uint8_t tos;
      uint16_t tot_len;
      uint16_t id;
      uint16_t frag_off_flags;
      uint8_t ttl;
      uint8_t protocol;
      uint16_t check;
      addr saddr;
      addr daddr;
    };

    /** The full header including IP.  
      
        Nested headers are useful for size-calculations etc., but cumbersome for
        checksums, so we use both.*/
    struct full_header{
      Ethernet::header eth_hdr;
      ip_header ip_hdr;
    };

  
    /** Upstream: Input from link layer. */
    int bottom(uint8_t* data, int len);
  
    /** Upstream: Outputs to transport layer*/
    inline void set_icmp_handler(subscriber s)
    { _icmp_handler = s; }
  
    inline void set_udp_handler(subscriber s)
    { _udp_handler = s; }
    
  
    inline void set_tcp_handler(subscriber s)
    { _tcp_handler = s; }
    
  
    /** Downstream: Delegate linklayer out */
    void set_linklayer_out(link_out s)
    { _linklayer_out = s; };
  
    /** Downstream: Receive data from above. */
    int transmit(addr source, addr dest, proto p,uint8_t* data, uint32_t len);

    /** Compute the IP4 header checksum */
    uint16_t checksum(ip_header* hdr);
  
    /** Initialize. Sets a dummy linklayer out. */
    IP4();
  
  
  private:    
  
    /** Downstream: Linklayer output delegate */
    link_out _linklayer_out;
  
    /** Upstream delegates */
    subscriber _icmp_handler;
    subscriber _udp_handler;
    subscriber _tcp_handler;  

  };


  /** Pretty printing to stream */
  std::ostream& operator<<(std::ostream& out, const IP4::addr& ip);

} // ~net
#endif
