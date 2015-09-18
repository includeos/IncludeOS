#include <net/ip6/udp6.hpp>

#include <iostream>
#include <net/util.hpp>

namespace net
{
  int UDPv6::bottom(std::shared_ptr<Packet>& pckt)
  {
    printf(">>> IPv6 -> UDPv6 bottom\n");
    auto P6 = std::static_pointer_cast<PacketUDP6>(pckt);
    
    printf(">>> src port: %u \t dst port: %u\n", P6->src_port(), P6->dst_port());
    printf(">>> length: %d   \t chksum: 0x%x\n", P6->length(), P6->checksum());
    
    port_t port = P6->dst_port();
    
    // check for listeners on dst port
    if (listeners.find(port) != listeners.end())
    {
      // make the call to the listener on that port
      return listeners[port](P6);
    }
    // was not forwarded, so just return -1
    printf("... dumping packet, no listeners\n");
    return -1;
  }
  
  int UDPv6::transmit(std::shared_ptr<Packet>& pckt)
  {
    return ip6_out(pckt);
  }
}
