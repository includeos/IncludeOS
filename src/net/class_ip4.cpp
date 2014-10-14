#include <os>
#include <net/class_ip4.hpp>



int IP4::bottom(uint8_t* data, int len){
  printf("<IP4 handler> got the data. I'm incompetent but I'll try:\n");
  
  header* hdr = (header*)((uint8_t*)data+sizeof(Ethernet::header));
  
  printf("\t Source IP: %1i.%1i.%1i.%1i Dest.IP: %1i.%1i.%1i.%1i  \n",
         hdr->saddr.part[0],hdr->saddr.part[1],hdr->saddr.part[2],hdr->saddr.part[3],
         hdr->daddr.part[0],hdr->daddr.part[1],hdr->daddr.part[2],hdr->daddr.part[3]
         );
  
  printf("\t Protocol: ");
  switch(hdr->protocol){
  case IP4_ICMP:
    printf("ICMP");
    break;
  case IP4_UDP:
    printf("UDP");
    break;
  case IP4_TCP:
    printf("TCP");
    break;
  default:
    printf("UNKNOWN");
    break;
  }
  printf("\n \t...No idea what do do with that.\n");

  return -1;
};

