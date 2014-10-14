#include <os>
#include <net/class_arp.hpp>

#include <vector>


int Arp::bottom(uint8_t* data, int len){
  printf("<ARP handler> got %i bytes of data \n",len);
  
  header* hdr= (header*) data;
  //printf("\t OPCODE: 0x%x \n",hdr->opcode);
  switch(hdr->opcode){
    
  case ARP_OP_REQUEST:
    printf("\t ARP REQUEST: \n");
    printf("\t Source IP: %i.%i.%i.%i \n",
           hdr->sipaddr.part[0], hdr->sipaddr.part[1],
           hdr->sipaddr.part[2], hdr->sipaddr.part[3]);
    
    printf("\t Destination IP: %i.%i.%i.%i \n",
           hdr->dipaddr.part[0], hdr->dipaddr.part[1],
           hdr->dipaddr.part[2], hdr->dipaddr.part[3]);
    
    printf("Packet remainder: \n"\
           "----------------------------------\n");
    for(int i = sizeof(header); i < len; i++)
      printf("%1x ",data[i]);    
    printf("\n----------------------------------\n");
    
    if (hdr->dipaddr == my_ip)
      arp_respond(hdr);    
    else 
      printf("\t NO MATCH for My IP. DROP!\n");        
    
    break;
    
  case ARP_OP_REPLY:    
    printf("\t ARP REPLY: \n");
    printf("\t IP: %i.%i.%i.%i belongs to "     \
           " %1x:%1x:%1x:%1x:%1x:%1x ",
           hdr->sipaddr.part[0], hdr->sipaddr.part[1],
           hdr->sipaddr.part[2], hdr->sipaddr.part[3],
           hdr->shwaddr.part[0], hdr->shwaddr.part[1], hdr->shwaddr.part[2],
           hdr->shwaddr.part[3], hdr->shwaddr.part[4], hdr->shwaddr.part[5]);
    break;
    
  default:
    printf("\t UNKNOWN OPCODE \n");
    break;
  }
  
  return 0;
};
  

extern "C" {
  unsigned long ether_crc(int length, unsigned char *data);
}

int Arp::arp_respond(header* hdr_in){
  printf("\t IP Match. Constructing ARP Reply \n");
  printf("\t ARP header size: %li \n",sizeof(header));
  
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
  memcpy((void*)&hdr->sipaddr, (void*)&my_ip, 4);
  memcpy((void*)&hdr->shwaddr, (void*)&my_mac, 6);
  memcpy((void*)&hdr->dhwaddr, (void*)&hdr_in->shwaddr,6);
  memcpy((void*)&hdr->dipaddr, (void*)&hdr_in->sipaddr,4);
  
  printf("\t My IP: %i.%i.%i.%i belongs to My Mac: "    \
         " %1x:%1x:%1x:%1x:%1x:%1x \n",
         hdr->sipaddr.part[0], hdr->sipaddr.part[1],
         hdr->sipaddr.part[2], hdr->sipaddr.part[3],
         hdr->shwaddr.part[0], hdr->shwaddr.part[1], hdr->shwaddr.part[2],
         hdr->shwaddr.part[3], hdr->shwaddr.part[4], hdr->shwaddr.part[5]);    
  
  // Fill out ethernet header. Here? Or in Ethernet?  
  memcpy((void*)&(hdr->ethhdr.src) ,(void*)&my_mac, 6);  
  memcpy((void*)&(hdr->ethhdr.dest),(void*)&(hdr_in->shwaddr), 6);
  printf("\t Eth.hdr - Src: %1x:%1x:%1x:%1x:%1x:%1x "\
         "Dest: %1x:%1x:%1x:%1x:%1x:%1x \n",
         hdr->ethhdr.src.part[0], hdr->ethhdr.src.part[1], hdr->ethhdr.src.part[2],
         hdr->ethhdr.src.part[3], hdr->ethhdr.src.part[4], hdr->ethhdr.src.part[5],
         hdr->ethhdr.dest.part[0], hdr->ethhdr.dest.part[1], hdr->ethhdr.dest.part[2],
         hdr->ethhdr.dest.part[3], hdr->ethhdr.dest.part[4], hdr->ethhdr.dest.part[5] 
         );    
  
  
  
  hdr->ethhdr.type = Ethernet::ETH_ARP;

  // Compute ethernet crc
  /*
  uint32_t crc = ether_crc(bufsize, buffer);
  uint32_t* CRC = (uint32_t*) buffer + sizeof(buffer) - 4;
  *CRC = crc;*/
  
  //printf("\tEthernet CRC: 0x%lx == 0x%lx\n", *CRC,crc);
  
  
  _linklayer_out(buffer, bufsize);
  
  return 0;
}


static int ignore(uint8_t* UNUSED(data),int len){
  printf("<ARP -> linklayer> Ignoring %ib output - no handler\n",len);
  return -1;
}

// Initialize
Arp::Arp(IP4::addr ip, Ethernet::addr mac): 
  my_ip(ip),my_mac(mac),_linklayer_out(delegate<int(uint8_t*,int)>(ignore)){}
