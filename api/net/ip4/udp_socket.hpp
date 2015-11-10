#pragma once
#include "udp.hpp"
#include "../socket.hpp"

namespace net
{
  class SocketUDP //: public Socket
  {
  public:
    typedef UDP::port port_t;
    typedef IP4::addr multicast_group_addr;
    
    typedef delegate<void(Socket&)> recvfrom_handler;
    typedef delegate<void(Socket&)> sendto_handler;
    
    
    SocketUDP(UDP&);
    SocketUDP(UDP&, IP4::addr addr, port_t port);
    
    int read(std::string& data);
    int write(const std::string& data);
    void close();
    
    void join(multicast_group_addr&);
    void leave(multicast_group_addr&);
    
    
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
    
    bool reuse_addr;
    bool loopback; // true means multicast data is looped back to sender
    
    friend class UDP;
  };
}
