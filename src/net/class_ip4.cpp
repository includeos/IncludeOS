//#define NDEBUG // Supress debugging
#include <os>
#include <net/class_ip4.hpp>



int IP4::bottom(uint8_t* data, int len){
  debug("<IP4 handler> got the data. I'm incompetent but I'll try:\n");
    
  ip_header* hdr = &((full_header*)data)->ip;
  
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



unsigned short checksum1(const char *buf, unsigned size)
{
  unsigned sum = 0;
  unsigned int i;

  /* Accumulate checksum */
  for (i = 0; i < size - 1; i += 2)
    {
      unsigned short word16 = *(unsigned short *) &buf[i];
      sum += word16;
    }

  /* Handle odd-sized case */
  if (size & 1)
    {
      unsigned short word16 = (unsigned char) buf[i];
      sum += word16;
    }

  /* Fold to get the ones-complement result */
  while (sum >> 16) sum = (sum & 0xFFFF)+(sum >> 16);

  /* Invert to get the negative in ones-complement arithmetic */
  return ~sum;
}


uint16_t IP4::checksum(ip_header* hdr){
  
  union sum{
    uint32_t whole;    
    uint16_t part[2];
  }sum32;
  sum32.whole = 0;
    
  for (uint16_t* i = (uint16_t*)hdr; i < (uint16_t*)hdr + (sizeof(ip_header)/2); i++){
    sum32.whole += *i;
  }
  
  // We're not checking for the odd-length case, since our heder is fixed @ 20b
  
  return ~(sum32.part[0]+sum32.part[1]);
}


int IP4::transmit(addr source, addr dest, proto p, uint8_t* data, uint32_t len){
  
  assert(len > sizeof(IP4::full_header));
  
  full_header* full_hdr = (full_header*) data;
  ip_header* hdr = &full_hdr->ip;

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
