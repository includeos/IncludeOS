#include <os>
#include <net/class_arp.hpp>



void Arp::handler(uint8_t* data, int len){
  printf("<ARP handler> got %i bytes of data \n",len);
};

Arp::Arp(Ethernet& eth){
  // Assign myself as ARP handler for eth
  eth.set_arp_handler(delegate<void(uint8_t*,int)>::from<Arp,
               &Arp::handler>(this));

}
