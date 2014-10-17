//#define NDEBUG // Supress debugging
#include <os>
#include <net/class_arp.hpp>

#include <vector>


int Arp::bottom(uint8_t* data, int len){
  debug("<ARP handler> got %i bytes of data \n",len);

  header* hdr= (header*) data;
  //debug("\t OPCODE: 0x%x \n",hdr->opcode);
  //std::cout << "Chaching IP " << hdr->sipaddr << " for " << hdr->shwaddr << std::endl;  
  printf("Have valid cache? %s \n",is_valid_cached(hdr->sipaddr) ? "YES":"NO");
  cache(hdr->sipaddr, hdr->shwaddr);
  
  switch(hdr->opcode){
    
  case ARP_OP_REQUEST:
    debug("\t ARP REQUEST: ");
    debug("%s is looking for %s \n",
          hdr->sipaddr.str().c_str(),hdr->dipaddr.str().c_str());
    
    if (hdr->dipaddr == _ip)
      arp_respond(hdr);    
    else{ debug("\t NO MATCH for My IP. DROP!\n"); }
        
    break;
    
  case ARP_OP_REPLY:    
    debug("\t ARP REPLY: %s belongs to %s\n",
          hdr->sipaddr.str().c_str(), hdr->shwaddr.str().c_str())
    break;
    
  default:
    debug("\t UNKNOWN OPCODE \n");
    break;
  }
  
  // Free the buffer (We're leaf node for this one's path)
  // @todo Freeing here corrupts the outgoing frame. Why?
  //free(data);
  
  return 0;
};
  

void Arp::cache(IP4::addr& ip, Ethernet::addr& mac){
  
  debug("Chaching IP %s for %s \n",ip.str().c_str(),mac.str().c_str());
  
  auto entry = _cache.find(ip);
  if (entry != _cache.end()){
    
    debug("Cached entry found: %s recorded @ %li. Updating timestamp \n",
          entry->second._mac.str().c_str(), entry->second._t);
    entry->second.update();
    
  }else _cache[ip] = mac;
  
}


bool Arp::is_valid_cached(IP4::addr& ip){
  auto entry = _cache.find(ip);
  return entry != _cache.end() 
    and (entry->second._t + cache_exp_t > OS::uptime());
}

extern "C" {
  unsigned long ether_crc(int length, unsigned char *data);
}

int Arp::arp_respond(header* hdr_in){
  debug("\t IP Match. Constructing ARP Reply \n");
  
  // Allocate send buffer
  int bufsize = sizeof(header);
  uint8_t* buffer = (uint8_t*)malloc(bufsize);
  header* hdr = (header*)buffer;
  
  // Populate header
  
  // Scalar 
  hdr->htype = hdr_in->htype;
  hdr->ptype = hdr_in->ptype;
  hdr->hlen_plen = hdr_in->hlen_plen;  
  hdr->opcode = ARP_OP_REPLY;
  
  hdr->dipaddr.whole = hdr_in->sipaddr.whole;
  hdr->sipaddr.whole = _ip.whole;
  
  // Composite  
  hdr->shwaddr.minor = _mac.minor;
  hdr->shwaddr.major = _mac.major;
  
  hdr->dhwaddr.minor = hdr_in->shwaddr.minor;
  hdr->dhwaddr.major = hdr_in->shwaddr.major;
  
  debug("\t My IP: %s belongs to My Mac: %s \n ",
        hdr->sipaddr.str().c_str(), hdr->shwaddr.str().c_str());
   
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
