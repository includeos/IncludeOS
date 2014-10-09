#ifndef CLASS_IP4_HPP
#define CLASS_IP4_HPP

#include <delegate>
#include <net/class_ethernet.hpp>


/** IP4 layer skeleton */
class IP4 {  
  
  // Outbound data goes through here
  //Ethernet& _eth;
  
  typedef delegate<int(uint8_t* data,int len)> subscriber;
  
  subscriber below;
  subscriber above;
  

public:
  
  union __attribute__((packed)) addr{
    uint8_t part[4];
    uint32_t whole;
    
    // Constructor
    addr(uint32_t ip){ whole=ip; };    
        
    inline bool operator==(addr& src)
    {
      return src.whole == whole; 
    }
  };
  
  /** Handle IPv4 packet. */
  int bottom(uint8_t* data, int len);

  void upstream(subscriber s);
  void downstream(subscriber s);
  
  
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
