#ifndef CLASS_UDP_HPP
#define CLASS_UDP_HPP

#include <net/class_ip4.hpp>

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
    
  typedef delegate<int(uint8_t* data,int len)> subscriber;

  /** Delegate output to network layer */
  void set_network_out(subscriber s);
  
private: 
  
  subscriber _network_layer_out;

  
};

#endif
