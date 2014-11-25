#define DEBUG // Allow debugging
//#define DEBUG2 // Allow debug lvl 2
#include <os>
#include <net/class_ip4.hpp>

using namespace net;


int IP4::bottom(std::shared_ptr<Packet>& pckt){
  debug2("<IP4 handler> got the data. \n");
    
  const uint8_t* data = pckt->buffer();
  ip_header* hdr = &((full_header*)data)->ip_hdr;
  
  debug2("\t Source IP: %s Dest.IP: %s type: ",
        hdr->saddr.str().c_str(), hdr->daddr.str().c_str() );
  
  switch(hdr->protocol){
  case IP4_ICMP:
    debug2("\t ICMP");
    _icmp_handler(pckt);
    break;
  case IP4_UDP:
    debug2("\t UDP");
    _udp_handler(pckt);
    break;
  case IP4_TCP:
    _tcp_handler(pckt);
    debug2("\t TCP");
    break;
  default:
    debug("\t UNKNOWN");
    break;
  }
  
  return -1;
};



uint16_t IP4::checksum(ip_header* hdr){
  return net::checksum((uint16_t*)hdr,sizeof(ip_header));
}

int IP4::transmit(std::shared_ptr<Packet>& pckt){

  //DEBUG Issue #102 :
  // Now _local_ip fails first, while _netmask fails if we remove local ip
  
  ASSERT(pckt->len() > sizeof(IP4::full_header));
  
  full_header* full_hdr = (full_header*) pckt->buffer();
  ip_header* hdr = &full_hdr->ip_hdr;

  hdr->version_ihl = 0x45; // IPv.4, Size 5 x 32-bit
  hdr->tos = 0; // Unused
  hdr->tot_len = __builtin_bswap16(pckt->len() - sizeof(Ethernet::header));
  hdr->id = 0; // Fragment ID (we don't fragment yet)
  hdr->frag_off_flags = 0x40; // "No fragment" flag set, nothing else
  hdr->ttl = 64; // What Linux / netcat does
  
  // Checksum is 0 while calculating
  hdr->check = 0;
  hdr->check = checksum(hdr);
  
  // Make sure it's right
  ASSERT(checksum(hdr) == 0);
    
  // Calculate next-hop
  ASSERT(pckt->next_hop().whole == 0);
  
  addr target_net;
  addr local_net;
  target_net.whole = hdr->daddr.whole & _netmask.whole;
  local_net.whole = _local_ip.whole & _netmask.whole;  

  debug2("<IP4 TOP> Next hop for %s, (netmask %s, local IP: %s, gateway: %s) == %s ",
        hdr->daddr.str().c_str(), 
        _netmask.str().c_str(), 
        _local_ip.str().c_str(),
        _gateway.str().c_str(),
        target_net == local_net ? "DIRECT" : "GATEWAY");
    
  pckt->next_hop(target_net == local_net ? hdr->daddr : _gateway);
  debug2("<IP4 transmit> my ip: %s, Next hop: %s \n",
        _local_ip.str().c_str(),
        pckt->next_hop().str().c_str());
  //debug("<IP4 TOP> - passing transmission to linklayer \n");
  return _linklayer_out(pckt);
};


/** Empty handler for delegates initialization */
int ignore_ip4(std::shared_ptr<Packet> UNUSED(pckt)){
  debug("<IP4> Empty handler. Ignoring.\n");
  return -1;
}

int ignore_transmission(std::shared_ptr<Packet> UNUSED(pckt)){

  debug("<IP4->Link layer> No handler - DROP!\n");
  return 0;
}

IP4::IP4(addr ip, addr netmask) :
  _local_ip(ip),
  _netmask(netmask),
  _gateway(),
  _linklayer_out(downstream(ignore_transmission)),
  _icmp_handler(upstream(ignore_ip4)),
  _udp_handler(upstream(ignore_ip4)),
  _tcp_handler(upstream(ignore_ip4))
{
  // Default gateway is addr 1 in the subnet.
  _gateway.whole = (ip.whole & netmask.whole) + __builtin_bswap32(1);
  
  debug("<IP4> Local IP @ 0x%lx, Netmask @ 0x%lx \n",
        (uint32_t)&_local_ip,
        (uint32_t)&_netmask);
}

std::ostream& operator<<(std::ostream& out,const IP4::addr& ip)  {
  return out << ip.str();
}
