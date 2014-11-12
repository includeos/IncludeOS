#define DEBUG // Allow debugging
#include <os>
#include <net/class_arp.hpp>

#include <vector>

using namespace net;

int Arp::bottom(std::shared_ptr<Packet> pckt)
{
  debug("<ARP handler> got %li bytes of data \n", pckt->len());

  header* hdr = (header*) pckt->buffer();
  //debug("\t OPCODE: 0x%x \n",hdr->opcode);
  //std::cout << "Chaching IP " << hdr->sipaddr << " for " << hdr->shwaddr << std::endl;  
  debug("Have valid cache? %s \n",is_valid_cached(hdr->sipaddr) ? "YES":"NO");
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
  
  return 0 + 0 * pckt->len(); // yep, it's what you think it is (and what's that?!)
};
  

void Arp::cache(IP4::addr& ip, Ethernet::addr& mac){
  
  debug("Chaching IP %s for %s \n",ip.str().c_str(),mac.str().c_str());
  
  auto entry = _cache.find(ip);
  if (entry != _cache.end()){
    
    debug("Cached entry found: %s recorded @ %li. Updating timestamp \n",
          entry->second._mac.str().c_str(), entry->second._t);
    
    // Update
    entry->second.update();
    
  }else _cache[ip] = mac; // Insert
  
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
                  uint8_t* UNUSED(data),int UNUSED(len)){
  debug("<ARP -> linklayer> Empty handler - DROP!\n");
  return -1;
}


int Arp::transmit(IP4::addr sip, IP4::addr dip, pbuf data, uint32_t len){
  debug("<ARP -> physical> Transmitting %li bytes to %s \n",
        len,dip.str().c_str());

  if (sip != _ip) {
    debug("<ARP -> physical> Not bound to source IP %s. My IP is %s. DROP!\n",
          sip.str().c_str(), _ip.str().c_str());            
    return -1;
  }
  
  if (!is_valid_cached(dip))
    panic("ARP cache missing for destination IP - and I don't know how to reslove yet\n");    

  auto mac = _cache[dip]._mac;
  debug("<ARP -> physical> Sending packet to %s \n",mac.str().c_str());

  return _linklayer_out(mac,Ethernet::ETH_IP4,data,len);
  
  return 0;
};

// Initialize
Arp::Arp(Ethernet::addr mac,IP4::addr ip): 
  _mac(mac), _ip(ip),
  _linklayer_out(delegate<int(Ethernet::addr,
                              Ethernet::ethertype,uint8_t*,int)>(ignore))
{}
