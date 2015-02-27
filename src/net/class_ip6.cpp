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
  
	int IP6::bottom(std::shared_ptr<Packet>& pckt)
	{
		//debug("<IP6 handler> got the data, but I'm clueless: DROP! \n");
		
    std::cout << ">>> IPv6 packet:" << std::endl;
    
    full_header& full = *(full_header*) pckt->buffer();
    
    header& hdr = full.ip6_hdr;
    
    std::cout << "version: " << hdr.getVersion() << " \t";
    std::cout << "class: " << hdr.getClass() << std::endl;
    
    
    return 0;
	};
  
  const std::string lut = "0123456789abcdef";
  
  std::string IP6::addr::to_string() const
  {
    std::string ret(48, '0');
    int counter = 0;
    
    uint8_t* octet = (uint8_t*) this;
    
    for (int i = 0; i < 16; i++)
    {
      ret[counter++] = lut[(*octet & 0x0F) >> 0];
      ret[counter++] = lut[(*octet & 0xF0) >> 8];
      ret[counter++] = ':';
    }
    ret.resize(counter-1);
    return ret;
  }
  
}
