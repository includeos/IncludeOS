#include <net/ip6/icmp6.hpp>

#include <iostream>

namespace net
{
  int ICMPv6::bottom(std::shared_ptr<Packet>& pckt)
  {
    std::cout << ">>> IPv6 -> ICMPv6 bottom" << std::endl;
    
    auto icmp = *reinterpret_cast<std::shared_ptr<PacketICMP6>*>(&pckt);
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
}
