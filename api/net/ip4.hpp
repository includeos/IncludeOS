#ifndef CLASS_IP4_HPP
#define CLASS_IP4_HPP

#include <delegate>

#include <net/inet_common.hpp>
#include <net/ethernet.hpp>

#include <iostream>
#include <string>

namespace net {

  class Packet;

  /** IP4 layer skeleton */
  class IP4 {  
  public:
  
    /** Known transport layer protocols. */
    enum proto{IP4_ICMP=1, IP4_UDP=17, IP4_TCP=6};
  
    /** Signature for output-delegates. */
    //typedef delegate<int(uint8_t* data,int len)> subscriber;  
  
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
      
      inline bool operator!=(const uint32_t src) const
      { return src != whole; }
      
      std::string str() const
      {
        std::string s; s.resize(16);
        sprintf((char*) s.c_str(), "%1i.%1i.%1i.%1i", 
                part[0], part[1], part[2], part[3]);
        return s;
      }
      
    };
    

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
    int bottom(Packet_ptr pckt);
    
    /** Upstream: Outputs to transport layer*/
    inline void set_icmp_handler(upstream s)
    { _icmp_handler = s; }
    
    inline void set_udp_handler(upstream s)
    { _udp_handler = s; }
    
    
    inline void set_tcp_handler(upstream s)
    { _tcp_handler = s; }
    
  
    /** Downstream: Delegate linklayer out */
    void set_linklayer_out(downstream s)
    { _linklayer_out = s; };
  
    /** Downstream: Receive data from above and transmit. 
        
        @note The following *must be set* in the packet:
        
        * Destination IP
        * Protocol
        
        Source IP *can* be set - if it's not, IP4 will set it.
        
    */
    int transmit(Packet_ptr pckt);

    /** Compute the IP4 header checksum */
    uint16_t checksum(ip_header* hdr);
    
    /**
     * \brief
     * Returns the IPv4 address associated with this interface
     * 
     **/
    addr local_ip() const
    {
      return _local_ip;
    }
    
    /** Initialize. Sets a dummy linklayer out. */
    IP4(addr ip, addr netmask);
    
  
  private:    
    addr _local_ip;
    addr _netmask;
    addr _gateway;
    
    /** Downstream: Linklayer output delegate */
    downstream _linklayer_out;
    
    /** Upstream delegates */
    upstream _icmp_handler;
    upstream _udp_handler;
    upstream _tcp_handler;
    
  };


  /** Pretty printing to stream */
  inline std::ostream& operator<<(std::ostream& out, const IP4::addr& ip)
  {
    return out << ip.str();
  }

} // ~net
#endif
