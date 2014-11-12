#define DEBUG // Allow debugging
#define DEBUG2

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

  debug2("<Ethernet OUT> Transmitting %i b, from %s -> %s. Type: %i \n",
        len,hdr->src.str().c_str(), hdr->dest.str().c_str(),hdr->type);
  
  return _physical_out(data, len);
}


int Ethernet::bottom(std::shared_ptr<net::Packet> pckt){  
  assert(pckt->len() > 0);

  header* eth = (header*) pckt->buffer();

  /** Do we pass on ethernet headers? Probably.
    data += sizeof(header);
    len -= sizeof(header);
  */
  
  
  // Print, for verification
  #ifdef DEBUG2
  char eaddr[] = "00:00:00:00:00:00";
  #endif
  debug2("<Ethernet IN> %s => %s , Eth.type: 0x%x ",
         ether2str(&eth->src,eaddr),
         ether2str(&eth->dest,eaddr),eth->type); 


  switch(eth->type){ 

  case ETH_IP4:
    debug2("IPv4 packet \n");
    return _ip4_handler(pckt);

  case ETH_IP6:
    debug2("IPv6 packet \n");
    return _ip6_handler(pckt);
    
  case ETH_ARP:
    debug2("ARP packet \n");
    return _arp_handler(pckt);
    
  case ETH_WOL:
    debug2("Wake-on-LAN packet \n");
    break;

  case ETH_VLAN:
    debug("VLAN tagged frame (not yet supported)");
    
  default:

    // This might be 802.3 LLC traffic
    if (__builtin_bswap16(eth->type) > 1500){
      debug("<Ethernet> UNKNOWN ethertype 0x%x\n",__builtin_bswap16(eth->type));
    }else{
      debug2("IEEE802.3 Length field: 0x%x\n",__builtin_bswap16(eth->type));
    }

    break;
    
  }
  
  return -1;
}

int ignore(std::shared_ptr<net::Packet> UNUSED(pckt)){
  debug("<Ethernet handler> Ignoring data (no real handler)\n");
  return -1;
};

Ethernet::Ethernet(addr mac) :
  _mac(mac),
  /** Default initializing to the empty handler. */
  _ip4_handler(upstream(ignore)),
  _ip6_handler(upstream(ignore)),
  _arp_handler(upstream(ignore))
{}


std::ostream& operator<<(std::ostream& out,Ethernet::addr& mac) {
  return out << mac.str();
}
