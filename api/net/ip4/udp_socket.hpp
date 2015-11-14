#pragma once
#include "udp.hpp"
#include "../socket.hpp"
#include <string>

namespace net
{
  class SocketUDP //: public Socket
  {
  public:
    typedef UDP::port port_t;
    typedef IP4::addr multicast_group_addr;
    
    typedef delegate<void(Socket&, const std::string&)> recvfrom_handler;
    typedef delegate<void(Socket&, const std::string&)> sendto_handler;
    
    
    SocketUDP(UDP&);
    SocketUDP(UDP&, port_t port);
    SocketUDP(const SocketUDP&) = delete;
    
    void onRead(recvfrom_handler func);
    void onWrite(sendto_handler func);
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
    int internal_read(std::shared_ptr<PacketUDP> udp);
    
    IP4::addr l_addr;
    port_t    l_port;
    IP4::addr p_addr;
    port_t    p_port;
    
    bool reuse_addr;
    bool loopback; // true means multicast data is looped back to sender
    
    friend class UDP;
  };
}
