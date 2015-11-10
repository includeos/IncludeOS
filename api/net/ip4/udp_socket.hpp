#pragma once
#include "udp.hpp"
#include "../socket.hpp"

namespace net
{
  class SocketUDP //: public Socket
  {
  public:
    typedef uint16_t port_t;
    
    SocketUDP& bind(port_t port);
    int read(std::string& data);
    int write(const std::string& data);
    void close();
    
    IP4::addr local_addr() const
    {
      return l_addr;
    }
    port_t local_port() const
    {
      return l_port;
    }
    IP4::addr peer_addr() const
    {
      return p_addr;
    }
    port_t peer_port() const
    {
      return p_port;
    }
    
    
  private:
    IP4::addr l_addr;
    port_t    l_port;
    IP4::addr p_addr;
    port_t    p_port;
    
  };
}
