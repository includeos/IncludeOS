#include <net/ip4/udp.hpp>

#ifndef NET_DHCP_DHCP4_HPP
#define NET_DHCP_DHCP4_HPP

#define DHCP_VEND_LEN	      308
#define BOOTP_MIN_LEN       300
#define DHCP_MIN_LEN        548
// some clients silently ignore responses less than 300 bytes
#define DEFAULT_PACKET_SIZE 300

namespace net
{
  struct dhcp_packet_t
  {
    static const int CHADDR_LEN =  16;
    static const int SNAME_LEN  =  64;
    static const int FILE_LEN   = 128;
    
    uint8_t  op;        // message opcode
    uint8_t  htype;     // hardware addr type
    uint8_t  hlen;      // hardware addr length
    uint8_t  hops;      // relay agent hops from client
    uint32_t xid;       // transaction ID
    uint16_t secs;      // seconds since start
    uint16_t flags;     // flag bits
    IP4::addr ciaddr;    // client IP address
    IP4::addr yiaddr;    // client IP address
    IP4::addr siaddr;    // IP address of next server
    IP4::addr giaddr;    // DHCP relay agent IP address
    uint8_t  chaddr[CHADDR_LEN];  // client hardware address
    uint8_t  sname[SNAME_LEN];    // server name
    uint8_t  file[FILE_LEN];      // BOOT filename
    uint32_t option_format;
    uint8_t  options[DHCP_VEND_LEN];
  };
  
  struct dhcp_option_t
  {
    uint8_t code;
    uint8_t length;
  };
  
}

#endif
