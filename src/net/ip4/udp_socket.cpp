#include <net/ip4/udp_socket.hpp>

namespace net
{
  using port_t = SocketUDP::port_t;
  
  SocketUDP::SocketUDP(UDP& _stack, port_t port)
    : stack(_stack), l_port(port)
  {
  }
  
  int SocketUDP::internal_read(std::shared_ptr<PacketUDP> udp)
  {
    const std::string buffer(
        udp->data(), 
        udp->data_length());
    return on_read(*this, udp->dst(), udp->dst_port(), buffer);
  }
  
  int SocketUDP::write(addr_t destIP, port_t port,
                       const std::string& buffer)
  {
    // create some packet p
    std::shared_ptr<PacketUDP> p;
    
    
  }
  
}
