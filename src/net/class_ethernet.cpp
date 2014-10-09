#include <os>
#include <net/class_ethernet.hpp>


char *ether2str(Ethernet::addr *hwaddr, char *s) {
  sprintf(s, "%02x:%02x:%02x:%02x:%02x:%02x",  
          hwaddr->addr[0], hwaddr->addr[1], hwaddr->addr[2], 
          hwaddr->addr[3], hwaddr->addr[4], hwaddr->addr[5]);
  return s;
}


int Ethernet::physical_in(uint8_t* data, int len){  
  assert(len > 0);

  printf("<Ethernet handler> parsing packet. \n ");  
  header* eth = (header*) data;

  /** Do we pass on ethernet headers? Probably.
    data += sizeof(header);
    len -= sizeof(header);
  */
  
  
  // Print, for verification
  char eaddr[] = "00:00:00:00:00:00";
  printf("\t             Eth. Source: %s \n",ether2str(&eth->src,eaddr));
  printf("\t             Eth. Dest. : %s \n",ether2str(&eth->dest,eaddr));
  printf("\t             Eth. Type  : 0x%x\n",eth->type); 
  

  switch(eth->type){ 

  case ETH_IP4:
    printf("\t             IPv4 packet \n");
    return _ip4_handler(data,len);

  case ETH_IP6:
    printf("\t             IPv6 packet \n");
    return _ip6_handler(data,len);
    
  case ETH_ARP:
    printf("\t             ARP packet \n");
    return _arp_handler(data,len);
    
  case ETH_WOL:
    printf("\t             Wake-on-LAN packet \n");
    break;
    
  default:
    printf("\t             UNKNOWN ethertype \n");
    break;
    
  }
  
  return -1;
}

int ignore(uint8_t* UNUSED(data), int UNUSED(len)){
  printf("<Ethernet handler> Ignoring data (no real handler)\n");
  return -1;
};

Ethernet::Ethernet() :
  /** Default initializing to the empty handler. */
  _ip4_handler(delegate<int(uint8_t*,int)>(ignore)),
  _ip6_handler(delegate<int(uint8_t*,int)>(ignore)),
  _arp_handler(delegate<int(uint8_t*,int)>(ignore))
{}
