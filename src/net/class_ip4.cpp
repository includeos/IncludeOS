#include <os>
#include <net/class_ip4.hpp>



int IP4::bottom(uint8_t* data, int len){
  printf("<IP4 handler> got the data. I'm incompetent but I'll try:\n");
  
  header* hdr = (header*)data;
  
  printf("\t Source IP: %1i.%1i.%1i.%1i Dest.IP: %1i.%1i.%1i.%1i  \n",
         hdr->saddr.part[0],hdr->saddr.part[1],hdr->saddr.part[2],hdr->saddr.part[3],
         hdr->daddr.part[0],hdr->daddr.part[1],hdr->daddr.part[2],hdr->daddr.part[3]
         );
  
  switch(hdr->protocol){
  case IP4_ICMP:
    _icmp_handler(data, len);
    break;
  case IP4_UDP:
    _udp_handler(data, len);
    break;
  case IP4_TCP:
    _tcp_handler(data, len);
    break;
  default:
    printf("UNKNOWN");
    break;
  }
  
  return -1;
};

int ignore_ip4(uint8_t* UNUSED(data), int UNUSED(len)){
  printf("<IP4> Empty handler. Ignoring.\n");
  return -1;
}

IP4::IP4() :
  _icmp_handler(delegate<int(uint8_t*,int)>(ignore_ip4)),
  _udp_handler(delegate<int(uint8_t*,int)>(ignore_ip4)),
  _tcp_handler(delegate<int(uint8_t*,int)>(ignore_ip4))
{}
