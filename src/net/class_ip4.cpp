//#define DEBUG // Allow debugging
#include <os>
#include <net/class_ip4.hpp>

using namespace net;


int IP4::bottom(std::shared_ptr<Packet> pckt){
  debug("<IP4 handler> got the data. \n");
    
  const uint8_t* data = pckt->buffer();
  ip_header* hdr = &((full_header*)data)->ip_hdr;
  
  debug("\t Source IP: %s Dest.IP: %s type: ",
        hdr->saddr.str().c_str(), hdr->daddr.str().c_str() );
  
  switch(hdr->protocol){
  case IP4_ICMP:
    debug("\t ICMP");
    _icmp_handler(pckt);
    break;
  case IP4_UDP:
    debug("\t UDP");
    _udp_handler(pckt);
    break;
  case IP4_TCP:
    _tcp_handler(pckt);
    debug("\t TCP");
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


int IP4::transmit(addr source, addr dest, proto p, uint8_t* data, uint32_t len){
  
  assert(len > sizeof(IP4::full_header));
  
  full_header* full_hdr = (full_header*) data;
  ip_header* hdr = &full_hdr->ip_hdr;

  hdr->version_ihl = 0x45; // IPv.4, Size 5 x 32-bit
  hdr->tos = 0; // Unused
  hdr->tot_len = __builtin_bswap16(len - sizeof(Ethernet::header));
  hdr->id = 0; // Fragment ID (we don't fragment yet)
  hdr->frag_off_flags = 0x40; // "No fragment" flag set, nothing else
  hdr->ttl = 64; // What Linux / netcat does
  hdr->protocol = p;   
  hdr->saddr.whole = source.whole;
  hdr->daddr.whole = dest.whole;  
  
  // Checksum is 0 while calculating
  hdr->check = 0;
  hdr->check = checksum(hdr);
  
  // Make sure it's right
  assert(checksum(hdr) == 0);


  debug("<IP4 TOP> - passing transmission to linklayer \n");
  return _linklayer_out(source,dest,data,len);
};


/** Empty handler for delegates initialization */
int ignore_ip4(std::shared_ptr<Packet> UNUSED(pckt)){
  debug("<IP4> Empty handler. Ignoring.\n");
  return -1;
}

int ignore_transmission(IP4::addr UNUSED(src),IP4::addr UNUSED(dst),
                        uint8_t* UNUSED(data), uint32_t UNUSED(len)){

  debug("<IP4->Link layer> No handler - DROP!\n");
  return 0;
}

IP4::IP4() :
  _linklayer_out(link_out(ignore_transmission)),
  _icmp_handler(upstream(ignore_ip4)),
  _udp_handler(upstream(ignore_ip4)),
  _tcp_handler(upstream(ignore_ip4))
{}

std::ostream& operator<<(std::ostream& out,const IP4::addr& ip)  {
  return out << ip.str();
}
