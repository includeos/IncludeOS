#ifndef CLASS_IP4_HPP
#define CLASS_IP4_HPP

#include <delegate>
#include <net/class_ethernet.hpp>

#include <iostream>
#include <string>

/** IP4 layer skeleton */
class IP4 {  
public:
  
  enum proto{IP4_ICMP=1, IP4_UDP=17, IP4_TCP=6};
  
  // Signature for output-delegates
  typedef delegate<int(uint8_t* data,int len)> subscriber;  

  union __attribute__((packed)) addr{
    uint8_t part[4];
    uint32_t whole;
    
    // Constructor
    //addr(uint32_t ip){ whole=ip; };    
        
    inline bool operator==(addr& src)
    {
      return src.whole == whole; 
    }
    
  };

  struct header{
    uint8_t version:4,ihl:4;
    uint8_t tos;
    uint16_t tot_len;
    uint16_t id;
    uint16_t frag_off;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t check;
    addr saddr;
    addr daddr;
  };
  
  /** Handle IPv4 packet. */
  int bottom(uint8_t* data, int len);

  void upstream(subscriber s);
  void downstream(subscriber s);

private:  
  
  // Outbound data goes through here
  //Ethernet& _eth;  
  
  subscriber below;
  subscriber above;
  

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

#endif
