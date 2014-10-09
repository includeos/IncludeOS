#include <os>
#include <net/class_arp.hpp>

#include <vector>


int Arp::bottom(uint8_t* data, int len){
  printf("<ARP handler> got %i bytes of data \n",len);
  
  header* hdr= (header*) data;
  
  printf("\t Source IP: %i.%i.%i.%i \n",
         hdr->sipaddr.part[0], hdr->sipaddr.part[1],
         hdr->sipaddr.part[2], hdr->sipaddr.part[3]);

  printf("\t Destination IP: %i.%i.%i.%i \n",
         hdr->dipaddr.part[0], hdr->dipaddr.part[1],
         hdr->dipaddr.part[2], hdr->dipaddr.part[3]);

  if (hdr->dipaddr == my_ip)
    printf("\t MATCHES my IP. Should respond, but don't know how\n");
  else 
    printf("\t NO MATCH for My IP. DROP!\n");

  return 0;
};


Arp::Arp(uint32_t ip):
  my_ip(ip)   //my_ip{192,168,0,11}
{}
