#define DEBUG // Allow debugging
#define DEBUG2 // Allow debug lvl 2
#include <os>
#include <net/ip4.hpp>
#include <net/ip4/packet_ip4.hpp>
#include <net/packet.hpp>

using namespace net;


int IP4::bottom(Packet_ptr pckt){
  debug2("<IP4 handler> got the data. \n");
    
  const uint8_t* data = pckt->buffer();
  ip_header* hdr = &((full_header*)data)->ip_hdr;
  
  debug2("\t Source IP: %s Dest.IP: %s type: ",
        hdr->saddr.str().c_str(), hdr->daddr.str().c_str() );
  
  switch(hdr->protocol){
  case IP4_ICMP:
    debug2("\t ICMP \n");
    icmp_handler_(pckt);
    break;
  case IP4_UDP:
    debug2("\t UDP \n");
    udp_handler_(pckt);
    break;
  case IP4_TCP:
    tcp_handler_(pckt);
    debug2("\t TCP \n");
    break;
  default:
    debug("\t UNKNOWN %i \n", hdr->protocol);
    break;
  }
  
  return -1;
};


uint16_t IP4::checksum(ip_header* hdr)
{
  return net::checksum((uint16_t*) hdr, sizeof(ip_header));
}

int IP4::transmit(Packet_ptr pckt)
{
  //DEBUG Issue #102 :
  // Now _local_ip fails first, while _netmask fails if we remove local ip
  assert(pckt->size() > sizeof(IP4::full_header));
  
  full_header* full_hdr = (full_header*) pckt->buffer();
  ip_header* hdr = &full_hdr->ip_hdr;
  
  /*
  hdr->version_ihl = 0x45; // IPv.4, Size 5 x 32-bit
  hdr->tos = 0; // Unused
  hdr->tot_len = __builtin_bswap16(pckt->size() - sizeof(Ethernet::header));
  hdr->id = 0; // Fragment ID (we don't fragment yet)
  hdr->frag_off_flags = 0x40; // "No fragment" flag set, nothing else
  hdr->ttl = 64; // What Linux / netcat does
  
  // Checksum is 0 while calculating
  hdr->check = 0;
  hdr->check = checksum(hdr);
  
  // Make sure it's right
  //assert(checksum(hdr) == 0);
    
  // Calculate next-hop
  //assert(pckt->next_hop().whole == 0);
  
  // Set destination address to "my ip" 
  // @TODO Don't know if this is good for routing...
  hdr->saddr.whole = local_ip_.whole;
  //ASSERT(! hdr->saddr.whole)
  
  std::shared_ptr<PacketIP4> p4 = 
      std::static_pointer_cast<PacketIP4> (pckt);
  
  debug2("<IP4 TOP> Dest: %s, Source: %s\n",
        p4->dst().str().c_str(), 
        p4->src().str().c_str());
  */
  
  // create local and target subnets
  addr target, local;
  target.whole = hdr->daddr.whole & netmask_.whole;
  local.whole  = local_ip_.whole  & netmask_.whole;  
  // compare subnets to know where to send packet
  pckt->next_hop(target == local ? hdr->daddr : gateway_);
  
  debug2("<IP4 TOP> Next hop for %s, (netmask %s, local IP: %s, gateway: %s) == %s\n",
        hdr->daddr.str().c_str(), 
        netmask_.str().c_str(), 
        local_ip_.str().c_str(),
        gateway_.str().c_str(),
        target == local ? "DIRECT" : "GATEWAY");
  
  debug2("<IP4 transmit> my ip: %s, Next hop: %s\n",
        local_ip_.str().c_str(),
        pckt->next_hop().str().c_str());
  
  return linklayer_out_(pckt);
}


/** Empty handler for delegates initialization */
int net::ignore_ip4_up(std::shared_ptr<Packet> UNUSED(pckt)){
  debug("<IP4> Empty handler. Ignoring.\n");
  return -1;
}

int net::ignore_ip4_down(std::shared_ptr<Packet> UNUSED(pckt)){

  debug("<IP4->Link layer> No handler - DROP!\n");
  return 0;
}

IP4::IP4(Inet<LinkLayer, IP4>& inet) :
  local_ip_(inet.ip_addr()),
  netmask_(inet.netmask())
{
  // Default gateway is addr 1 in the subnet.
  const uint32_t DEFAULT_GATEWAY = __builtin_bswap32(1);
  
  gateway_.whole = (local_ip_.whole & netmask_.whole) | DEFAULT_GATEWAY;
  
  debug("<IP4> Local IP @ %p, Netmask @ %p \n",
        (void*) &local_ip_,
        (void*) &netmask_);
}
