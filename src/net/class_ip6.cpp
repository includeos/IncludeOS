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
  
	int IP6::bottom(std::shared_ptr<Packet>& pckt)
	{
    std::cout << ">>> IPv6 packet:" << std::endl;
    
    full_header& full = *(full_header*) pckt->buffer();
    
    header& hdr = full.ip6_hdr;
    
    std::cout << "version: " << hdr.version() << " \t";
    std::cout << "class: " << hdr.tclass() << std::endl;
    
    std::cout << "payload: " << hdr.size() << " bytes" << std::endl;
    
    std::cout << "next hdr: " << protocol_name(hdr.next()) << std::endl;
    std::cout << "hoplimit: " << hdr.hoplimit() << " hops" << std::endl;
    
    std::cout << "a src: " << hdr.source() << std::endl;
    std::cout << "a dst: " << hdr.dest() << std::endl;
    
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
