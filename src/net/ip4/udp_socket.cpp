#include <net/ip4/udp_socket.hpp>
#include <memory>

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
    return on_read(*this, udp->src(), udp->src_port(), buffer);
  }
  
  void SocketUDP::packet_init(
      std::shared_ptr<PacketUDP> p, addr_t destIP, port_t port, uint16_t length)
  {
    p->init();
    p->header().sport = this->l_port; //<some random number>
    p->header().dport = port;
    p->set_src(stack.local_ip());
    p->set_dst(destIP);
    p->set_length(length);
    
    assert(p->data_length() == length);
  }
  
  int SocketUDP::write(addr_t destIP, port_t port,
                       const std::string& string_buffer)
  {
    int rem = string_buffer.size();
    // source buffer
    uint8_t* buffer = (uint8_t*) string_buffer.data();
    // the maximum we can write per packet:
    const int WRITE_MAX = Packet::MTU - PacketUDP::HEADERS_SIZE;
    
    while (rem >= WRITE_MAX)
    {
      // create and fill buffer (at payload position)
      uint8_t* pbuf = new uint8_t[Packet::MTU];
      memcpy(pbuf + PacketUDP::HEADERS_SIZE, buffer, WRITE_MAX);
      
      // create some packet p (and convert it to PacketUDP)
      // TODO: UDP& or Inet& stack into SocketUDP
      /*
      auto p = std::make_shared<Packet>(
          pbuf, Packet::MTU, Packet::DOWNSTREAM);
      auto p2 = std::static_pointer_cast<PacketUDP>(p);
      // initialize packet with several infos
      packet_init(p2, destIP, port, WRITE_MAX);
      // ship the packet
      stack.transmit(p2);
      
      // next buffer part
      buffer += WRITE_MAX;  rem -= WRITE_MAX;
      */
    }
    if (rem)
    {
      // copy remainder
      size_t size   = PacketUDP::HEADERS_SIZE + rem;
      uint8_t* pbuf = new uint8_t[size];
      memcpy(pbuf + PacketUDP::HEADERS_SIZE, buffer, rem);
      
      // create some packet p
      /*
      auto p = std::make_shared<Packet>(
          pbuf, size, Packet::DOWNSTREAM);
      auto p2 = std::static_pointer_cast<PacketUDP>(p);
      // initialize packet with several infos
      packet_init(p2, destIP, port, rem);
      // ship the packet
      stack.transmit(p2);
      */
    }
    return -1;
  } // write()
  
}
