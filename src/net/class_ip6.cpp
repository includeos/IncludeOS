//#define DEBUG // Allow debugging
#include <os>
#include <net/class_ip6.hpp>

#include <assert.h>

namespace net
{
  IP6::IP6(const IP6::addr& lo)
    : local(lo)
  {
    assert(sizeof(addr)   == 16);
    assert(sizeof(header) == 40);
    assert(sizeof(options_header) == 8);
    
  }
  
  void fix(IP6::addr& a)
  {
    for (int i = 0; i < 16; i += 2)
    {
      uint8_t s = a.i8[i];
      
      a.i8[i]   = a.i8[i+1];
      a.i8[i+1] = s;
    }
  }
  
  uint8_t IP6::parse6(uint8_t*& reader, uint8_t next)
  {
    switch (next)
    {
    case PROTO_HOPOPT:
    case PROTO_OPTSv6:
      {
        std::cout << ">>> IPv6 options header " << protocol_name(next) << std::endl;
        
        options_header& opts = *(options_header*) reader;
        reader += opts.size();
        
        std::cout << "OPTSv6 size: " << opts.size() << std::endl;
        std::cout << "OPTSv6 ext size: " << opts.extended() << std::endl;
        
        next = opts.next();
        std::cout << "OPTSv6 next: " << protocol_name(next) << std::endl;
      } break;
    case PROTO_ICMPv6:
      {
        std::cout << ">>> IPv6 ICMP header " << protocol_name(next) << std::endl;
        
        icmp_header& icmp = *(icmp_header*) reader;
        std::cout << "ICMPv6 type: " << icmp.type() << " --> ";
        
        switch (icmp.type())
        {
          /// error codes ///
        case 1:
          std::cout << "Destination Unreachable " << icmp.code() << ": ";
          switch (icmp.code())
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
          std::cout << "Time exceeded " << icmp.code() << ": ";
          switch (icmp.code())
          {
          case 0:
            std::cout << "Hop limit exceeded in traffic"; break;
          case 1:
            std::cout << "Fragment reassembly time exceeded"; break;
          default:
            std::cout << "ERROR Invalid ICMP type";
          } break;
        case 4:
          std::cout << "Parameter problem " << icmp.code() << ": ";
          switch (icmp.code())
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
          std::cout << "Unknown type, code = " << icmp.code();
        }
        std::cout << std::endl;
        
        intptr_t chksum = icmp.checksum();
        std::cout << "ICMPv6 checksum: " << (void*) chksum << std::endl;
        
        // payload comes after this
        return PROTO_NoNext;
        
      } break;
    default:
      std::cout << "Not parsing " << protocol_name(next) << std::endl;
    }
    
    return next;
  }
  
	int IP6::bottom(std::shared_ptr<Packet>& pckt)
	{
    std::cout << ">>> IPv6 packet:" << std::endl;
    
    uint8_t* reader = pckt->buffer();
    full_header& full = *(full_header*) reader;
    reader += sizeof(full_header);
    
    header& hdr = full.ip6_hdr;
    
    std::cout << "IPv6 v: " << hdr.version() << " \t";
    std::cout << "class: " << hdr.tclass() << " \t";
    
    std::cout << "size: " << hdr.size() << " bytes" << std::endl;
    
    uint8_t next = hdr.next();
    std::cout << "IPv6 next hdr: " << protocol_name(next) << std::endl;
    std::cout << "IPv6 hoplimit: " << hdr.hoplimit() << " hops" << std::endl;
    
    std::cout << "IPv6 src: " << hdr.source() << std::endl;
    std::cout << "IPv6 dst: " << hdr.dest() << std::endl;
    
    while (next != PROTO_NoNext)
    {
      next = parse6(reader, next);
    }
    
    std::cout << std::endl;
    return 0;
	};
  
  const std::string lut = "0123456789abcdef";
  
  std::string IP6::addr::to_string() const
  {
    std::string ret(48, '0');
    int counter = 0;
    
    uint8_t* octet = (uint8_t*) i8;
    
    for (int i = 0; i < 16; i++)
    {
      ret[counter++] = lut[(octet[i] & 0xF0) >> 4];
      ret[counter++] = lut[(octet[i] & 0x0F) >> 0];
      if (i & 1)
        ret[counter++] = ':';
    }
    ret.resize(counter-1);
    return ret;
  }
  
}
