#define DEBUG // Allow debugging

#include <os>
#include <net/class_ethernet.hpp>

using namespace net;

// FROM SanOS
//
// ether_crc
//

#define ETHERNET_POLYNOMIAL 0x04c11db7U

extern "C" {
  unsigned long ether_crc(int length, unsigned char *data) {
    int crc = -1;
    
    while (--length >= 0) {
      unsigned char current_octet = *data++;
    int bit;
    for (bit = 0; bit < 8; bit++, current_octet >>= 1) {
      crc = (crc << 1) ^ ((crc < 0) ^ (current_octet & 1) ? ETHERNET_POLYNOMIAL : 0);
    }
    }
    
    return crc;
  }
  


  char *ether2str(Ethernet::addr *hwaddr, char *s) {
    sprintf(s, "%02x:%02x:%02x:%02x:%02x:%02x",  
            hwaddr->part[0], hwaddr->part[1], hwaddr->part[2], 
            hwaddr->part[3], hwaddr->part[4], hwaddr->part[5]);
    return s;
  }
  
}

int Ethernet::transmit(addr mac, ethertype type, uint8_t* data, int len){
  header* hdr = (header*)data;
  memcpy((void*)&hdr->src, (void*)&_mac, 6);
  memcpy((void*)&hdr->dest, (void*)&mac, 6);
  hdr->type = type;

  debug2("<Ethernet->Phys> Transmitting %i b, from %s -> %s. Type: %i \n",
        len,hdr->src.str().c_str(), hdr->dest.str().c_str(),hdr->type);
  
  return _physical_out(data, len);
}

int Ethernet::physical_in(uint8_t* data, int len){  
  assert(len > 0);

  debug2("<Ethernet handler> parsing packet. \n ");  
  header* eth = (header*) data;

  /** Do we pass on ethernet headers? Probably.
    data += sizeof(header);
    len -= sizeof(header);
  */
  
  
  // Print, for verification
  #ifdef DEBUG2
  char eaddr[] = "00:00:00:00:00:00";
  #endif
  debug2("\t             Eth. Source: %s \n",ether2str(&eth->src,eaddr));
  debug2("\t             Eth. Dest. : %s \n",ether2str(&eth->dest,eaddr));
  debug2("\t             Eth. Type  : 0x%x\n",eth->type); 


  switch(eth->type){ 

  case ETH_IP4:
    debug2("\t             IPv4 packet \n");
    return _ip4_handler(data,len);

  case ETH_IP6:
    debug2("\t             IPv6 packet \n");
    return _ip6_handler(data,len);
    
  case ETH_ARP:
    debug2("\t             ARP packet \n");
    return _arp_handler(data,len);
    
  case ETH_WOL:
    debug2("\t             Wake-on-LAN packet \n");
    break;
    
  default:
    debug("<Ethernet> UNKNOWN ethertype \n");
    break;
    
  }
  
  return -1;
}

int ignore(uint8_t* UNUSED(data), int UNUSED(len)){
  debug("<Ethernet handler> Ignoring data (no real handler)\n");
  return -1;
};

Ethernet::Ethernet(addr mac) :
  _mac(mac),
  /** Default initializing to the empty handler. */
  _ip4_handler(delegate<int(uint8_t*,int)>(ignore)),
  _ip6_handler(delegate<int(uint8_t*,int)>(ignore)),
  _arp_handler(delegate<int(uint8_t*,int)>(ignore))
{}


std::ostream& operator<<(std::ostream& out,Ethernet::addr& mac) {
  return out << mac.str();
}
