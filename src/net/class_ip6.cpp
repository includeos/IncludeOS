//#define DEBUG // Allow debugging
#include <os>
#include <net/class_ip6.hpp>
#include <net/ip6/icmp6.hpp>
#include <net/ip6/udp6.hpp>

#include <assert.h>

namespace net
{
  IP6::IP6(const IP6::addr& lo)
  //  : local(lo)
  {
    int ttt;
    printf("IP6: stack: %p\n", &ttt);
    
    printf("IP6 constructor, addr = %s\n", lo.to_string().c_str());
    assert(sizeof(addr)   == 16);
    assert(sizeof(header) == 40);
    assert(sizeof(options_header) == 8);
    
    this->local = lo;
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
      auto it = proto_handlers.find(next);
      if (it != proto_handlers.end())
      {
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
  
  const std::string lut = "0123456789abcdef";
  
  std::string IP6::addr::to_string() const
  {
    std::string ret(48, '0');
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
  
}
