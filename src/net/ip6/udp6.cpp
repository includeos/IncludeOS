#include <net/ip6/udp6.hpp>

#include <iostream>
#include <net/util.hpp>

namespace net
{
  int UDPv6::bottom(std::shared_ptr<Packet>& pckt)
  {
    std::cout << ">>> IPv6 -> UDPv6 bottom" << std::endl;
    
    auto& P6 = *reinterpret_cast<std::shared_ptr<PacketUDP6>*>(&pckt);
    
    std::cout << "src port: " << P6->getSourcePort() << std::endl;
    std::cout << "dst port: " << P6->getDestPort() << std::endl;
    
    std::cout << "length: " << P6->getLength() << "b" << std::endl;
    printf("chksum: 0x%x\n", P6->getChecksum());
    
    uint16_t length = P6->getLength();
    const char* data = P6->getData();
    
    std::cout << "data: " << std::string(data, length) << std::endl;
    return -1;
  }
}
