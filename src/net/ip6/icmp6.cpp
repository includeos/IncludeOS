#include <net/ip6/icmp6.hpp>

#include <iostream>
#include <net/ip6/ip6.hpp>
#include <alloca.h>

namespace net
{
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
    case 128:
      return "Echo request";
    case 129:
      return "Echo reply";
      
      /// multicast feature ///
    case 130:
      return "Multicast listener query";
    case 131:
      return "Multicast listener report";
    case 132:
      return "Multicast listener done";
      
      /// neighbor discovery protocol ///
    case 133:
      return "NDP Router solicitation request";
    case 134:
      return "NDP Router advertisement";
    case 135:
      return "NDP Neighbor solicitation request";
    case 136:
      return "NDP Neighbor advertisement";
    case 137:
      return "NDP Redirect message";
      
    case 143:
      return "Multicast Listener Discovery (MLDv2) reports (RFC 3810)";
      
    default:
      return "Unknown type: " + std::to_string((int) type);
    }
  }
  
  
  int ICMPv6::bottom(std::shared_ptr<Packet>& pckt)
  {
    auto icmp = std::static_pointer_cast<PacketICMP6>(pckt); //*reinterpret_cast<std::shared_ptr<PacketICMP6>*>(&pckt);
    
    switch (icmp->type())
    {
    case ECHO_REQUEST:
      echo_request(icmp);
      return -1;
      
    default:
      return -1;
      std::cout << ">>> IPv6 -> ICMPv6 bottom" << std::endl;
      std::cout << "ICMPv6 type " << (int) icmp->type() << ": " << code_string(icmp->type(), icmp->code()) << std::endl;
      
      // show correct checksum
      intptr_t chksum = icmp->checksum();
      std::cout << "ICMPv6 checksum: " << (void*) chksum << std::endl;
      
      // show our recalculated checksum
      icmp->header().checksum_ = 0;
      chksum = checksum(icmp);
      std::cout << "ICMPv6 our estimate: " << (void*) chksum << std::endl;
      return -1;
    }
  }
  
  uint16_t ICMPv6::checksum(std::shared_ptr<PacketICMP6>& pckt)
  {
    IP6::full_header& full = *(IP6::full_header*) pckt->buffer();
    IP6::header& hdr = full.ip6_hdr;
    
    // ICMP message length + pseudo header
    uint16_t datalen = sizeof(pseudo_header) + hdr.size() - sizeof(IP6::header);
    
    // allocate it on stack
    char* data = (char*) alloca(datalen + 16);
    
    // unfortunately we also need to guarantee SSE aligned
    #define	P2ALIGN  (x, align)    ((x) & -(align))
    #define	P2ROUNDUP(x, align)    (-(-(x) & -(align)))
    
    data = (char*) P2ROUNDUP((intptr_t) data, 16);
    
    pseudo_header& phdr = *(pseudo_header*) data;
    phdr.src = hdr.src;
    phdr.dst = hdr.dst;
    phdr.zeroes[0] = 0;
    phdr.zeroes[1] = 0;
    phdr.zeroes[2] = 0;
    phdr.next = hdr.next();
    
    // normally we would start at &icmp_echo::type, but
    // it is the first element of the icmp message
    memcpy(data + sizeof(pseudo_header), pckt->payload(),
        datalen - sizeof(pseudo_header));
    
    // calculate csum and free it on return
    uint16_t chksum = net::checksum((uint16_t*) data, datalen);
    
    return chksum;
  }
  
  int ICMPv6::echo_request(std::shared_ptr<PacketICMP6>& pckt)
  {
    uint8_t* reader = pckt->buffer();
    IP6::full_header& full = *(IP6::full_header*) reader;
    IP6::header& hdr = full.ip6_hdr;
    
    // retrieve source and destination addresses
    IP6::addr src = hdr.source();
    //IP6::addr dst = hdr.dest();
    
    // switch them around!
    hdr.src = this->localIP;
    hdr.dst = src;
    
    icmp6_echo* icmp = (icmp6_echo*) &pckt->header();
    printf("\n*** RECEIVED ECHO type=%d 0x%x\n", icmp->type, htons(icmp->checksum));
    
    // set to ICMP Echo Reply (129)
    icmp->type     = ECHO_REPLY;
    icmp->checksum = 0;
    
    // calculate and set checksum
    icmp->checksum = htons(checksum(pckt));
    
    // send packet downstream
    // NOTE: *** ALLOCATED ON STACK *** -->
    auto original = std::static_pointer_cast<Packet>(pckt);
    // NOTE: *** ALLOCATED ON STACK *** <--
    return ip6_out(original);
  }
  
}
