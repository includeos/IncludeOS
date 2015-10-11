#define DEBUG
#include <net/ip6/icmp6.hpp>
#include <net/ip6/ndp.hpp>

#include <iostream>
#include <net/ip6/ip6.hpp>
#include <alloca.h>
#include <assert.h>

namespace net
{
  // internal implementation of handler for ICMP type 128 (echo requests)
  int echo_request(ICMPv6&, std::shared_ptr<PacketICMP6>& pckt);
  int neighbor_solicitation(ICMPv6& caller, std::shared_ptr<PacketICMP6>& pckt);
  
  ICMPv6::ICMPv6(IP6::addr& local_ip)
    : localIP(local_ip)
  {
    // install default handler for echo requests
    listen(ECHO_REQUEST, echo_request);
    // install default handler for neighbor solicitation requests
    listen(ND_NEIGHB_SOL, neighbor_solicitation);
  }
  
  // static function that returns a textual representation of
  // the @code and @type of an ICMP message
  std::string ICMPv6::code_string(uint8_t type, uint8_t code)
  {
    switch (type)
    {
      /// error codes ///
    case 1:
      /// delivery problems ///
      switch (code)
      {
      case 0:
        return "No route to destination";
      case 1:
        return "Communication with dest administratively prohibited";
      case 2:
        return "Beyond scope of source address";
      case 3:
        return "Address unreachable";
      case 4:
        return "Port unreachable";
      case 5:
        return "Source address failed ingress/egress policy";
      case 6:
        return "Reject route to destination";
      case 7:
        return "Error in source routing header";
      default:
        return "ERROR Invalid ICMP type";
      }
    case 2:
      /// size problems ///
      return "Packet too big";
      
    case 3:
      /// time problems ///
      switch (code)
      {
      case 0:
        return "Hop limit exceeded in traffic";
      case 1:
        return "Fragment reassembly time exceeded";
      default:
        return "ERROR Invalid ICMP code";
      }
    case 4:
      /// parameter problems ///
      switch (code)
      {
      case 0:
        return "Erroneous header field";
      case 1:
        return "Unrecognized next header";
      case 2:
        return "Unrecognized IPv6 option";
      default:
        return "ERROR Invalid ICMP code";
      }
      
      /// echo feature ///
    case ECHO_REQUEST:
      return "Echo request";
    case ECHO_REPLY:
      return "Echo reply";
      
      /// multicast feature ///
    case 130:
      return "Multicast listener query";
    case 131:
      return "Multicast listener report";
    case 132:
      return "Multicast listener done";
      
      /// neighbor discovery protocol ///
    case ND_ROUTER_SOL:
      return "NDP Router solicitation request";
    case ND_ROUTER_ADV:
      return "NDP Router advertisement";
    case ND_NEIGHB_SOL:
      return "NDP Neighbor solicitation request";
    case ND_NEIGHB_ADV:
      return "NDP Neighbor advertisement";
    case ND_REDIRECT:
      return "NDP Redirect message";
      
    case 143:
      return "Multicast Listener Discovery (MLDv2) reports (RFC 3810)";
      
    default:
      return "Unknown type: " + std::to_string((int) type);
    }
  }
  
  int ICMPv6::bottom(std::shared_ptr<Packet>& pckt)
  {
    auto icmp = std::static_pointer_cast<PacketICMP6>(pckt);
    
    type_t type = icmp->type();
    
    if (listeners.find(type) != listeners.end())
    {
      return listeners[type](*this, icmp);
    }
    else
    {
      debug(">>> IPv6 -> ICMPv6 bottom (no handler installed)\n");
      debug("ICMPv6 type %d: %s\n", 
          (int) icmp->type(), code_string(icmp->type(), icmp->code()).c_str());
      
      /*
      // show correct checksum
      intptr_t chksum = icmp->checksum();
      std::cout << "ICMPv6 checksum: " << (void*) chksum << std::endl;
      
      // show our recalculated checksum
      icmp->header().checksum_ = 0;
      chksum = checksum(icmp);
      std::cout << "ICMPv6 our estimate: " << (void*) chksum << std::endl;
      */
      return -1;
    }
  }
  int ICMPv6::transmit(std::shared_ptr<PacketICMP6>& pckt)
  {
    // NOTE: *** OBJECT CREATED ON STACK *** -->
    auto original = std::static_pointer_cast<PacketIP6>(pckt);
    // NOTE: *** OBJECT CREATED ON STACK *** <--
    return ip6_out(original);
  }
  
  uint16_t ICMPv6::checksum(std::shared_ptr<PacketICMP6>& pckt)
  {
    IP6::full_header& full = *(IP6::full_header*) pckt->buffer();
    IP6::header& hdr = full.ip6_hdr;
    
    // ICMP message + pseudo header
    uint16_t datalen = hdr.size() + sizeof(pseudo_header);
    
    // allocate it on stack
    char* data = (char*) alloca(datalen + 16);
    // unfortunately we also need to guarantee SSE aligned
    data = (char*) ((intptr_t) (data+16) & ~15); // P2ROUNDUP((intptr_t) data, 16);
    // verify that its SSE aligned
    assert(((intptr_t) data & 15) == 0);
    
    // ICMP checksum is done with a pseudo header
    // consisting of src addr, dst addr, message length (32bits)
    // 3 zeroes (8bits each) and id of the next header
    pseudo_header& phdr = *(pseudo_header*) data;
    phdr.src = hdr.src;
    phdr.dst = hdr.dst;
    phdr.len = htonl(hdr.size());
    phdr.zeros[0] = 0;
    phdr.zeros[1] = 0;
    phdr.zeros[2] = 0;
    phdr.next = hdr.next();
    //assert(hdr.next() == 58); // ICMPv6
    
    // normally we would start at &icmp_echo::type, but
    // it is after all the first element of the icmp message
    memcpy(data + sizeof(pseudo_header), pckt->payload(),
        datalen - sizeof(pseudo_header));
    
    // calculate csum and free data on return
    return net::checksum(data, datalen);
  }
  
  // internal implementation of handler for ICMP type 128 (echo requests)
  int echo_request(ICMPv6& caller, std::shared_ptr<PacketICMP6>& pckt)
  {
    ICMPv6::echo_header* icmp = (ICMPv6::echo_header*) pckt->payload();
    printf("*** Custom handler for ICMP ECHO REQ type=%d 0x%x\n", icmp->type, htons(icmp->checksum));
    
    // set the hoplimit manually to the very standard 64 hops
    pckt->set_hoplimit(64);
    
    // set to ICMP Echo Reply (129)
    icmp->type = ICMPv6::ECHO_REPLY;
    
    if (pckt->dst().is_multicast())
    {
      // We won't be changing source address for multicast ping
      printf("Multicast ping6: no change for source and dest\n");
    }
    else
    {
      printf("Normal ping6: source is us\n");
      printf("src is %s\n", pckt->src().str().c_str());
      printf("dst is %s\n", pckt->dst().str().c_str());
      
      printf("multicast is %s\n", IP6::addr::link_all_nodes.str().c_str());
      // normal ping: send packet to source, from us
      pckt->set_dst(pckt->src());
      pckt->set_src(caller.local_ip());
    }
    // calculate and set checksum
    // NOTE: do this after changing packet contents!
    icmp->checksum = 0;
    icmp->checksum = ICMPv6::checksum(pckt);
    
    // send packet downstream
    return caller.transmit(pckt);
  }
  int neighbor_solicitation(ICMPv6& caller, std::shared_ptr<PacketICMP6>& pckt)
  {
    NDP::neighbor_sol* sol = (NDP::neighbor_sol*) pckt->payload();
    
    printf("ICMPv6 NDP Neighbor solicitation request\n");
    printf(">> target: %s\n", sol->target.str().c_str());
    printf(">>\n");
    printf(">> source: %s\n", pckt->src().str().c_str());
    printf(">> dest:   %s\n", pckt->dst().str().c_str());
    
    // perhaps we should answer
    
    
    return -1;
  }
  
  void ICMPv6::discover()
  {
    // ether-broadcast an IPv6 packet to all routers
    // IPv6mcast_02: 33:33:00:00:00:02
    auto pckt = IP6::create(
        IP6::PROTO_ICMPv6,
        Ethernet::addr::IPv6mcast_02, 
        IP6::addr::link_unspecified);
    
    // RFC4861 4.1. Router Solicitation Message Format
    pckt->set_hoplimit(255);
    
    NDP::router_sol* ndp = (NDP::router_sol*) pckt->payload();
    // set to Router Solicitation Request
    ndp->type = ICMPv6::ND_ROUTER_SOL;
    ndp->code = 0;
    ndp->checksum = 0;
    ndp->reserved = 0;
    
    auto icmp = std::static_pointer_cast<PacketICMP6> (pckt);
    
    // source and destination addresses
    icmp->set_src(IP6::addr::link_unspecified);
    icmp->set_dst(IP6::addr::link_all_routers);
    
    // ICMP header length field
    icmp->set_length(sizeof(NDP::router_sol));
    
    // calculate and set checksum
    // NOTE: do this after changing packet contents!
    ndp->checksum = ICMPv6::checksum(icmp);
    
    this->transmit(icmp);
  }
  
  
}
