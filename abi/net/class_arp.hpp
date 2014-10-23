#ifndef CLASS_ARP_HPP
#define CLASS_ARP_HPP

#include <class_os.hpp>
#include <delegate>
#include <net/class_ethernet.hpp>
#include <net/class_ip4.hpp>
#include <map>

namespace net {

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

    /** Temporary type of protocol buffer. @todo encapsulate.*/
    typedef uint8_t* pbuf;

  
    /** Handle incoming ARP packet. */
    int bottom(uint8_t* data, int len);
    
    /** Delegate link-layer output. */
    inline void set_linklayer_out(delegate<int(Ethernet::addr,Ethernet::ethertype,uint8_t*,int)> link){
      _linklayer_out = link;
    };

    /** Downstream transmission. */
    int transmit(IP4::addr sip, IP4::addr dip, pbuf data, uint32_t len);
  
    inline IP4::addr& ip() { return _ip; }

    Arp(Ethernet::addr,IP4::addr);
  
  private: 
  
    // Needs to know which mac address to put in header->swhaddr
    Ethernet::addr _mac;

    // Needs to know which IP to respond to
    IP4::addr _ip;
  
    // Outbound data goes through here
    delegate<int(Ethernet::addr,Ethernet::ethertype,uint8_t*,int)> _linklayer_out;

    /** Cache entries are just macs and timestamps */
    struct cache_entry{
      Ethernet::addr _mac;
      uint32_t _t;
    
      cache_entry(){}; // map needs empty constructor (we have no emplace yet)
      cache_entry(Ethernet::addr mac) :_mac(mac),_t(OS::uptime()) {};
      cache_entry(const cache_entry& cpy)
      { _mac.major = cpy._mac.major; _mac.minor = cpy._mac.minor; _t = cpy._t; }
      void update() { _t = OS::uptime(); }
    };
  
    // The arp cache
    std::map<IP4::addr,cache_entry> _cache;
  
    // Arp cache expires after cache_exp_t seconds
    uint16_t cache_exp_t = 60 * 5;

    /** Cache IP resolution. */
    void cache(IP4::addr&, Ethernet::addr&);
  
    /** Checks if an IP is cached and not expired */
    bool is_valid_cached(IP4::addr&);

    /** Arp resolution. */
    Ethernet::addr& resolve(IP4::addr&);
  
    int arp_respond(header* hdr_in);
    int arp_request(IP4::addr ip);

  };

} // ~net
#endif
