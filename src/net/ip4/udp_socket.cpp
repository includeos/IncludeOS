#include <net/ip4/udp_socket.hpp>

namespace net
{
  using port_t = SocketUDP::port_t;
  
  SocketUDP::SocketUDP(UDP&, port_t port)
  {
    // create/retrieve packet
    const uint8_t* buffer = new 
    auto p = std::make_shared<PacketUDP> (buffer,
  }
  
}
