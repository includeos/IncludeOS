#include <os>
#include <net/class_arp.hpp>




void Arp::handler(uint8_t* data, int len){
  printf("<ARP handler> got %i bytes of data \n",len);
  
  header* hdr= (header*) data;
  
  printf("\t Source IP: %i.%i.%i.%i \n",
         hdr->sipaddr._byte[0], hdr->sipaddr._byte[1],
         hdr->sipaddr._byte[2], hdr->sipaddr._byte[3]);

  printf("\t Destination IP: %i.%i.%i.%i \n",
         hdr->dipaddr._byte[0], hdr->dipaddr._byte[1],
         hdr->dipaddr._byte[2], hdr->dipaddr._byte[3]);

  
};


Arp::Arp(Ethernet& eth)
  : _eth(eth)
{
  // Assign myself as ARP handler for eth
  eth.set_arp_handler(delegate<void(uint8_t*,int)>::from<Arp,
                      &Arp::handler>(this));
  
}
