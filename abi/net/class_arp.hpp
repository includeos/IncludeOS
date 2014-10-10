#ifndef CLASS_ARP_HPP
#define CLASS_ARP_HPP

#include <delegate>
#include <net/class_ethernet.hpp>
#include <net/class_ip4.hpp>

class Arp {
     
public:
  
  // Arp opcodes (Big-endian)
  #define ARP_OP_REQUEST 0x100
  #define ARP_OP_REPLY 0x200
  
  struct __attribute__((packed)) header {
    Ethernet::header ethhdr;   // Ethernet header
    uint16_t htype;            // Hardware type
    uint16_t ptype;            // Protocol type
    uint16_t hlen_plen;        // Protocol address length
    uint16_t opcode;           // Opcode
    Ethernet::addr shwaddr;    // Source mac
    IP4::addr sipaddr;         // Source ip
    Ethernet::addr dhwaddr;    // Target mac
    IP4::addr dipaddr;         // Target ip
  };

  
  /** Handle incoming ARP packet. */
  int bottom(uint8_t* data, int len);
  
  /** Constructor. Requires an IP address to answer to. */
  Arp(IP4::addr ip, Ethernet::addr mac);
  Arp(uint32_t ip);
  
  /** Delegate link-layer output. */
  inline void set_linklayer_out(delegate<int(uint8_t*,int)> link){
    _linklayer_out = link;
  };
                           
  
private: 
  
  IP4::addr my_ip; //{192,168,0,11};
  Ethernet::addr my_mac;
  
  // Outbound data goes through here
  delegate<int(uint8_t*,int)> _linklayer_out;
  
  int arp_respond(header* hdr_in);
  int arp_request(IP4::addr ip);

};

#endif
