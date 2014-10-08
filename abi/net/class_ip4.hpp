#ifndef CLASS_IP4_HPP
#define CLASS_IP4_HPP

#include <delegate>
#include <net/class_ethernet.hpp>


/** IP4 layer skeleton */
class IP4 {
  
  // Outbound data goes through here
  Ethernet& _eth;

public:
  
  /** Handle IPv4 packet. */
  void handler(uint8_t* data, int len);
  
  /** Constructor. Requires ethernet to latch on to. */
  IP4(Ethernet& eth);
};

#endif
