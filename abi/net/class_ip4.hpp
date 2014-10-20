#ifndef CLASS_IP4_HPP
#define CLASS_IP4_HPP

#include <delegate>
#include <net/class_ethernet.hpp>

#include <iostream>
#include <string>

/** IP4 layer skeleton */
class IP4 {  
public:
  
  /** Known transport layer protocols. */
  enum proto{IP4_ICMP=1, IP4_UDP=17, IP4_TCP=6};
  
  /** Signature for output-delegates. */
  typedef delegate<int(uint8_t* data,int len)> subscriber;  

  
  /** IP4 address */
  union __attribute__((packed)) addr{
    uint8_t part[4];
    uint32_t whole;
    
    // Constructor
    //addr(uint32_t ip){ whole=ip; };    
        
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
  
  /** Delegate type for linklayer out */
  typedef delegate<int(addr,addr,uint8_t* data,int len)> link_out;      
  
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

  struct full_header{
    Ethernet::header eth_hdr;
    ip_header ip;
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
  
  inline addr ip(){
    return _ip;
  }

  IP4(addr ip);
  
  
private:  
  
  IP4::addr _ip; //{192,168,0,11};

  /** Downstream: Linklayer output delegate */
  link_out _linklayer_out;
  
  /** Upstream delegates */
  subscriber _icmp_handler;
  subscriber _udp_handler;
  subscriber _tcp_handler;
  

  /** IP stack sketch
        
  class IP_stack {
    //Stack objects:
    eth,arp,ip4,tcp,http;
    
  public:
    IP_stack(Nic nic){
    
    
    // Upstream  
    nic.upstream(eth);
    eth.upstream(ip4_bottom)
    ip4.upstream(tcp_bottom);  
    tcp.upstream(80,http_bottom)
    // <==> tcp.listen(80,http)
    http.upstream(app)

    // Downstream
    http.downstream(tcp_top)  
    tcp.downstream(ip4_top)
    ip4.downstream(arp_top);
    arp.downstream(eth);
    eth.downstream(nic_top);
    
    }
  */
  
  
  
  /** Constructor. Requires ethernet to latch on to. */
  //IP4(Ethernet& eth);

};


///std::ostream& operator<<(std::ostream& out, IP4::addr& ip);
std::ostream& operator<<(std::ostream& out, const IP4::addr& ip);


#endif
