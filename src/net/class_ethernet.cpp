// #define DEBUG // Allow debugging
// #define DEBUG2

#include <os>
#include <net/class_ethernet.hpp>

using namespace net;

const Ethernet::addr Ethernet::addr::multicast_frame{minor: 0x0000, major: 0x00000001};
const Ethernet::addr Ethernet::addr::broadcast_frame{minor: 0xFFFF, major: 0xFFFFFFFF};


int Ethernet::transmit(std::shared_ptr<Packet>& pckt){
  header* hdr = (header*)pckt->buffer();

  // Verify ethernet header
  ASSERT(hdr->dest.major != 0 || hdr->dest.minor !=0);
  ASSERT(hdr->type != 0);
  
  // Add source address
  hdr->src.major = _mac.major;
  hdr->src.minor = _mac.minor;

  debug2("<Ethernet OUT> Transmitting %li b, from %s -> %s. Type: %i \n",
         pckt->len(),hdr->src.str().c_str(), hdr->dest.str().c_str(),hdr->type);
  
  return _physical_out(pckt);
}


int Ethernet::bottom(std::shared_ptr<net::Packet>& pckt){  
  ASSERT(pckt->len() > 0);

  header* eth = (header*) pckt->buffer();

  /** Do we pass on ethernet headers? Probably.
    data += sizeof(header);
    len -= sizeof(header);
  */
    
  debug2("<Ethernet IN> %s => %s , Eth.type: 0x%x ",
         eth->src.str().c_str(),
         eth->dest.str().c_str(),eth->type); 


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
