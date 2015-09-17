#include <net/ip6/icmp6.hpp>

#include <iostream>
#include <net/ip6/ip6.hpp>
#include <alloca.h>

namespace net
{
  int ICMPv6::bottom(std::shared_ptr<Packet>& pckt)
  {
    std::cout << ">>> IPv6 -> ICMPv6 bottom" << std::endl;
    
    auto& icmp = *reinterpret_cast<std::shared_ptr<PacketICMP6>*>(&pckt);
    std::cout << "ICMPv6 type: " << icmp->type() << " --> ";
    
    switch (icmp->type())
    {
      /// error codes ///
    case 1:
      std::cout << "Destination Unreachable " << icmp->code() << ": ";
      switch (icmp->code())
      {
      case 0:
        std::cout << "No route to destination"; break;
      case 1:
        std::cout << "Communication with dest administratively prohibited"; break;
      case 2:
        std::cout << "Beyond scope of source address"; break;
      case 3:
        std::cout << "Address unreachable"; break;
      case 4:
        std::cout << "Port unreachable"; break;
      case 5:
        std::cout << "Source address failed ingress/egress policy"; break;
      case 6:
        std::cout << "Reject route to destination"; break;
      case 7:
        std::cout << "Error in source routing header"; break;
      default:
        std::cout << "ERROR Invalid ICMP type";
      } break;
    case 2:
      std::cout << "Packet too big";
      break;
    case 3:
      std::cout << "Time exceeded " << icmp->code() << ": ";
      switch (icmp->code())
      {
      case 0:
        std::cout << "Hop limit exceeded in traffic"; break;
      case 1:
        std::cout << "Fragment reassembly time exceeded"; break;
      default:
        std::cout << "ERROR Invalid ICMP type";
      } break;
    case 4:
      std::cout << "Parameter problem " << icmp->code() << ": ";
      switch (icmp->code())
      {
      case 0:
        std::cout << "Erroneous header field"; break;
      case 1:
        std::cout << "Unrecognized next header"; break;
      case 2:
        std::cout << "Unrecognized IPv6 option"; break;
      default:
        std::cout << "ERROR Invalid ICMP type";
      } break;
      
      /// echo feature ///
    case 128:
      std::cout << "Echo request";
      echo_request(pckt);
      break;
    case 129:
      std::cout << "Echo reply";
      break;
      
      /// multicast feature ///
    case 130:
      std::cout << "Multicast listener query";
      break;
    case 131:
      std::cout << "Multicast listener report";
      break;
    case 132:
      std::cout << "Multicast listener done";
      break;
      
      /// neighbor discovery protocol ///
    case 133:
      std::cout << "NDP Router solicitation request";
      break;
    case 134:
      std::cout << "NDP Router advertisement";
      break;
    case 135:
      std::cout << "NDP Neighbor solicitation request";
      break;
    case 136:
      std::cout << "NDP Neighbor advertisement";
      break;
    case 137:
      std::cout << "NDP Redirect message";
      break;
      
    case 143:
      std::cout << "Multicast Listener Discovery (MLDv2) reports (RFC 3810)";
      break;
      
    default:
      std::cout << "Unknown type, code = " << icmp->code();
    }
    std::cout << std::endl;
    
    intptr_t chksum = icmp->checksum();
    std::cout << "ICMPv6 checksum: " << (void*) chksum << std::endl;
    return -1;
  }
  
  uint16_t ICMPv6::checksum(std::shared_ptr<Packet>& pckt)
  {
    IP6::full_header& full = *(IP6::full_header*) pckt->buffer();
    IP6::header& hdr = full.ip6_hdr;
    
    // ICMP message length + pseudo header
    uint16_t datalen = sizeof(pseudo_header) + hdr.size() - sizeof(IP6::header);
    
    // allocate it on stack
    char* data = (char*) alloca(datalen);
    
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
    return net::checksum((uint16_t*) data, datalen);
  }
  
  void ICMPv6::echo_request(std::shared_ptr<Packet>& pckt)
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
    
    auto icmp_packet = *reinterpret_cast<std::shared_ptr<PacketICMP6>*>(&pckt);
    
    icmp6_echo* icmp = (icmp6_echo*) &icmp_packet->header();
    printf("\n*** RECEIVED ECHO type=%d 0x%x\n", icmp->type, htons(icmp->checksum));
    
    // set to ICMP Echo Reply (129)
    icmp->type     = 129;
    icmp->checksum =   0;
    
    // calculate and set checksum
    icmp->checksum = htons(checksum(pckt));
    
    // send packet downstream
    ip6_out(pckt);
  }
  
}
