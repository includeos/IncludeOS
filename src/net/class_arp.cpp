#define NDEBUG // Supress debugging
#include <os>
#include <net/class_arp.hpp>

#include <vector>


int Arp::bottom(uint8_t* data, int len){
  debug("<ARP handler> got %i bytes of data \n",len);
  
  header* hdr= (header*) data;
  //debug("\t OPCODE: 0x%x \n",hdr->opcode);
  switch(hdr->opcode){
    
  case ARP_OP_REQUEST:
    debug("\t ARP REQUEST: ");
    debug("%i.%i.%i.%i is looking for ",
           hdr->sipaddr.part[0], hdr->sipaddr.part[1],
           hdr->sipaddr.part[2], hdr->sipaddr.part[3]);
    
    debug(" %i.%i.%i.%i \n",
           hdr->dipaddr.part[0], hdr->dipaddr.part[1],
           hdr->dipaddr.part[2], hdr->dipaddr.part[3]);
    
    /*
    debug("Packet remainder: \n"\
           "----------------------------------\n");
    for(int i = sizeof(header); i < len; i++)
      debug("%1x ",data[i]);    
    debug("\n----------------------------------\n");*/
    
    if (hdr->dipaddr == _ip)
      arp_respond(hdr);    
    else 
      debug("\t NO MATCH for My IP. DROP!\n");        
    
    break;
    
  case ARP_OP_REPLY:    
    debug("\t ARP REPLY: ");
    debug("  %i.%i.%i.%i belongs to "          \
           " %1x:%1x:%1x:%1x:%1x:%1x ",
           hdr->sipaddr.part[0], hdr->sipaddr.part[1],
           hdr->sipaddr.part[2], hdr->sipaddr.part[3],
           hdr->shwaddr.part[0], hdr->shwaddr.part[1], hdr->shwaddr.part[2],
           hdr->shwaddr.part[3], hdr->shwaddr.part[4], hdr->shwaddr.part[5]);
    break;
    
  default:
    debug("\t UNKNOWN OPCODE \n");
    break;
  }
  
  return 0;
};
  

extern "C" {
  unsigned long ether_crc(int length, unsigned char *data);
}

int Arp::arp_respond(header* hdr_in){
  debug("\t IP Match. Constructing ARP Reply \n");
  
  // Allocate send buffer
  int bufsize = sizeof(header) + 12;
  uint8_t* buffer = (uint8_t*)malloc(bufsize);
  header* hdr = (header*)buffer;
  
  // Copy some values
  
  // Scalar 
  hdr->htype = hdr_in->htype;
  hdr->ptype = hdr_in->ptype;
  hdr->hlen_plen = hdr_in->hlen_plen;  
  hdr->opcode = ARP_OP_REPLY;

  // Composite
  memcpy((void*)&hdr->sipaddr, (void*)&_ip, 4);
  memcpy((void*)&hdr->dipaddr, (void*)&hdr_in->sipaddr,4);
  memcpy((void*)&hdr->shwaddr, (void*)&_mac, 6);
  memcpy((void*)&hdr->dhwaddr, (void*)&hdr_in->shwaddr,6);  


  debug("\t My IP: %i.%i.%i.%i belongs to My Mac: "    \
         " %1x:%1x:%1x:%1x:%1x:%1x \n",
         hdr->sipaddr.part[0], hdr->sipaddr.part[1],
         hdr->sipaddr.part[2], hdr->sipaddr.part[3],
         hdr->shwaddr.part[0], hdr->shwaddr.part[1], hdr->shwaddr.part[2],
         hdr->shwaddr.part[3], hdr->shwaddr.part[4], hdr->shwaddr.part[5]); 
   
  _linklayer_out(hdr->dhwaddr, Ethernet::ETH_ARP, buffer, bufsize);
  
  return 0;
}


static int ignore(Ethernet::addr UNUSED(mac),Ethernet::ethertype UNUSED(etype),
                  uint8_t* UNUSED(data),int len){
  debug("<ARP -> linklayer> Ignoring %ib output - no handler\n",len);
  return -1;
}

// Initialize
Arp::Arp(Ethernet::addr mac,IP4::addr ip): 
  _mac(mac), _ip(ip),
  _linklayer_out(delegate<int(Ethernet::addr,
                              Ethernet::ethertype,uint8_t*,int)>(ignore))
{}
