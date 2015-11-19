#ifndef NET_DHCP_DHCP4_HPP
#define NET_DHCP_DHCP4_HPP

namespace net
{
  class DHCP4
  {
    static const int BOOTREQUEST = 1;
    static const int BOOTREPLY   = 2;
    
    uint8_t  op;
    uint8_t  htype;
    uint8_t  hlen;
    uint8_t  hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t  chaddr[16];
    uint8_t  sname[64];
    uint8_t  file[128];
    uint8_t  options[0];
  };
}

#endif
