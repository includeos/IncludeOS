#ifndef CLASS_UDP_HPP
#define CLASS_UDP_HPP

#include <net/class_ip4.hpp>
#include <map>

class UDP{
public:

  /** UDP Header */
  struct header{
    IP4::header ip_hdr;
    uint16_t sport;
    uint16_t dport;
    uint16_t length;
    uint16_t checksum;
  }__attribute__((packed));

  /** Input from network layer */
  int bottom(uint8_t* data, int len);
    
  typedef delegate<int(uint8_t* data,int len)> listener;
  typedef delegate<int(IP4::addr,pbuf& buf)> network_out;
  
  /** Delegate output to network layer */
  inline void set_network_out(network_out del)
  { _network_layer_out = del; }
  
  /** Listen to a port. 
      
      @note Any previous listener will be evicted (it's all your service) */
  void listen(uint16_t port, listener s);
  
private: 
  
  network_out _network_layer_out;
  std::map<uint16_t, listener> ports;
  
};

#endif
