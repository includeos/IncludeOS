#ifndef CLASS_ICMP_HPP
#define CLASS_ICMP_HPP

class ICMP{
public:

  /** Input from network layer */
  int bottom(uint8_t* data, int len);
    
  typedef delegate<int(uint8_t* data,int len)> subscriber;

  /** Delegate output to network layer */
  void set_network_out(subscriber s);
  
private: 
  
  subscriber _network_layer_out;

  
};

#endif
