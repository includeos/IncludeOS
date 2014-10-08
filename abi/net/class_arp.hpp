#ifndef CLASS_ARP_HPP
#define CLASS_ARP_HPP

#include <delegate>
#include <net/class_ethernet.hpp>
#include <net/class_ip4.hpp>

class Arp {
  
  // Outbound data goes through here
  Ethernet& _eth;

public:
  
  struct __attribute__((packed)) header {
    Ethernet::header ethhdr;   // Ethernet header
    uint16_t htype;            // Hardware type
    uint16_t ptype;            // Protocol type
    uint16_t hlen_plen;        // Protocol address length
    uint16_t opcode;           // Opcode
    Ethernet::addr shwaddr;    // Source hardware address
    IP4::addr sipaddr;         // Source protocol address
    Ethernet::addr dhwaddr;    // Target hardware address
    IP4::addr dipaddr;         // Target protocol address
  };

  
  /** Handle ARP packet. */
  void handler(uint8_t* data, int len);

  /** Constructor. Requires ethernet to latch on to. */
  Arp(Ethernet& eth);
  
};

#endif
