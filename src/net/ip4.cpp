#define DEBUG // Allow debugging
#define DEBUG2 // Allow debug lvl 2
#include <os>
#include <net/ip4.hpp>
#include <net/ip4/packet_ip4.hpp>
#include <net/packet.hpp>

using namespace net;

const IP4::addr IP4::INADDR_ANY{{0,0,0,0}};

int IP4::bottom(Packet_ptr pckt){
  debug2("<IP4 handler> got the data. \n");
    
  const uint8_t* data = pckt->buffer();
  ip_header* hdr = &((full_header*)data)->ip_hdr;
  
  debug2("\t Source IP: %s Dest.IP: %s\n",
        hdr->saddr.str().c_str(), hdr->daddr.str().c_str() );
  
  switch(hdr->protocol){
  case IP4_ICMP:
    debug2("\t Type: ICMP \n");
    icmp_handler_(pckt);
    break;
  case IP4_UDP:
    debug2("\t Type: UDP \n");
    udp_handler_(pckt);
    break;
  case IP4_TCP:
    tcp_handler_(pckt);
    debug2("\t Type: TCP \n");
    break;
  default:
    debug("\t Type: UNKNOWN %i \n", hdr->protocol);
    break;
  }
  
  return -1;
}

uint16_t IP4::checksum(ip_header* hdr)
{
  return net::checksum((uint16_t*) hdr, sizeof(ip_header));
}

int IP4::transmit(Packet_ptr pckt)
{
  assert(pckt->size() > sizeof(IP4::full_header));    
    
  full_header* full_hdr = (full_header*) pckt->buffer();
  ip_header* hdr = &full_hdr->ip_hdr;
  
  auto ip4_pckt = std::static_pointer_cast<PacketIP4>(pckt);
  ip4_pckt->make_flight_ready();
  
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
  
  debug2("<IP4 transmit> my ip: %s, Next hop: %s, Packet size: %i IP4-size: %i\n",
	 local_ip_.str().c_str(),
	 pckt->next_hop().str().c_str(),
	 pckt->size(),
	 ip4_pckt->ip4_segment_size()
	 );
  
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
  const uint32_t DEFAULT_GATEWAY = htonl(1);
  
  gateway_.whole = (local_ip_.whole & netmask_.whole) | DEFAULT_GATEWAY;
  
  debug("<IP4> Local IP @ %p, Netmask @ %p \n",
        (void*) &local_ip_,
        (void*) &netmask_);
}
