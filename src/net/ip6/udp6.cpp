#include <net/ip6/udp6.hpp>

#include <iostream>
#include <net/util.hpp>

namespace net
{
  int UDPv6::handler(std::shared_ptr<PacketUDP6>& pckt)
  {
    udp6_header& hdr = pckt->header();
    
    std::cout << ">>> UDPv6 packet" << std::endl;
    std::cout << "src port: " << htons(hdr.src_port) << std::endl;
    std::cout << "dst port: " << htons(hdr.dst_port) << std::endl;
    
    std::cout << "length: " << htons(hdr.length) << "b" << std::endl;
    printf("chksum: 0x%x\n", htons(hdr.chksum));
    
    uint16_t length = htons(hdr.length);
    char* data = (char*) pckt->payload() + sizeof(udp6_header);
    
    std::cout << "data: " << std::string(data, length) << std::endl;
    return -1;
  }
}
