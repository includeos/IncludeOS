//#define NDEBUG // Supress debugging
#include <os>
#include <net/class_ip4.hpp>



int IP4::bottom(uint8_t* data, int len){
  debug("<IP4 handler> got the data. I'm incompetent but I'll try:\n");
  
  header* hdr = (header*)data;
  
  debug("\t Source IP: %s Dest.IP: %s \n",
        hdr->saddr.str().c_str(), hdr->daddr.str().c_str() );
  
  switch(hdr->protocol){
  case IP4_ICMP:
    _icmp_handler(data, len);
    break;
  case IP4_UDP:
    _udp_handler(data, len);
    break;
  case IP4_TCP:
    _tcp_handler(data, len);
    break;
  default:
    debug("UNKNOWN");
    break;
  }
  
  return -1;
};


int IP4::transmit(addr source, addr dest, proto p, uint8_t* data, uint32_t len){
  
  assert(len > sizeof(IP4::header));
  
  IP4::header* hdr = (IP4::header*) data;

  hdr->version_ihl = 0x45; // IPv.4, Size 5 x 32-bit
  hdr->tos = 0; // Unused
  hdr->tot_len = __builtin_bswap16(len - sizeof(Ethernet::header));
  hdr->frag_off_flags = 0x40; // "No fragment" flag set, nothing else
  hdr->ttl = 64; // What Linux / netcat does
  hdr->protocol = p; 
  
  hdr->saddr.whole = source.whole;
  hdr->daddr.whole = dest.whole;  
  
  debug("<IP4 TOP> - passing transmission to linklayer \n");
  return _linklayer_out(source,dest,data,len);
};


int ignore_ip4(uint8_t* UNUSED(data), int UNUSED(len)){
  debug("<IP4> Empty handler. Ignoring.\n");
  return -1;
}

int ignore_transmission(IP4::addr UNUSED(src),IP4::addr UNUSED(dst),
                        uint8_t* UNUSED(data), uint32_t UNUSED(len)){

  debug("<IP4->Link layer> No handler - DROP!\n");
  return 0;
}

IP4::IP4(addr ip) :
  _ip(ip),
  _linklayer_out(link_out(ignore_transmission)),
  _icmp_handler(subscriber(ignore_ip4)),
  _udp_handler(subscriber(ignore_ip4)),
  _tcp_handler(subscriber(ignore_ip4))
{}

std::ostream& operator<<(std::ostream& out,const IP4::addr& ip)  {
  return out << ip.str();
}
