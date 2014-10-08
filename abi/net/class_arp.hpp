#ifndef CLASS_ARP_HPP
#define CLASS_ARP_HPP

#include <delegate>
#include <net/class_ethernet.hpp>

class Arp {
  
public:
  
  /** Handle ARP packet. */
  void handler(uint8_t* data, int len);

  /** Constructor. Requires ethernet to latch on to. */
  Arp(Ethernet& eth);
  
};

#endif
