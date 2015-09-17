//#define DEBUG // Allow debugging
#include <os>
#include <net/ip6/ip6.hpp>

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
      break;
    case PROTO_UDP:
      break;
      
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
    
    std::cout << "IPv6 v: " << (int) hdr.version() << " \t";
    std::cout << "class: "  << (int) hdr.tclass()  << " \t";
    
    std::cout << "size: " << hdr.size() << " bytes" << std::endl;
    
    uint8_t next = hdr.next();
    std::cout << "IPv6 next hdr: " << protocol_name(next)  << std::endl;
    std::cout << "IPv6 hoplimit: " << (int) hdr.hoplimit() << " hops" << std::endl;
    
    std::cout << "IPv6 src: " << hdr.source() << std::endl;
    std::cout << "IPv6 dst: " << hdr.dest() << std::endl;
    
    while (next != PROTO_NoNext)
    {
      auto it = proto_handlers.find(next);
      if (it != proto_handlers.end())
      {
        // forward packet to handler
        pckt->_payload = reader;
        return it->second(pckt);
      }
      else
        // just print information
        next = parse6(reader, next);
    }
    
    std::cout << std::endl;
    return 0;
	};
  
  static const std::string lut = "0123456789abcdef";
  
  std::string IP6::addr::to_string() const
  {
    std::string ret(40, 0);
    int counter = 0;
    
    const uint8_t* octet = i8;
    
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
  
  int IP6::transmit(std::shared_ptr<Packet>& pckt)
  {
    full_header& full = *(full_header*) pckt->buffer();
    header& hdr = full.ip6_hdr;
    
    // verify that it is IPv6 packet
    //ASSERT(hdr->dest.major != 0 || hdr->dest.minor !=0);
    //ASSERT(hdr->type != 0);
    
    // set source IPv6-address directly (?)
    //hdr.src = this->local;
    
    debug2("<IP6 OUT> Transmitting %li b, from %s -> %s\n",
           pckt->len(), hdr.src.str().c_str(), hdr.dst.str().c_str());
    
    return _linklayer_out(pckt);
  }
  
}
