#ifndef NET_IP4_UDP_SOCKET_HPP
#define NET_IP4_UDP_SOCKET_HPP
#include "udp.hpp"
#include <string>

namespace net
{
  template <class T>
  class Socket;
  
  template<>
  class Socket<UDP>
  {
  public:
    typedef UDP::port port_t;
    typedef IP4::addr addr_t;
    typedef IP4::addr multicast_group_addr;
    
    typedef delegate<int(Socket<UDP>&, addr_t, port_t, const char*, int)> recvfrom_handler;
    typedef delegate<int(Socket<UDP>&, addr_t, port_t, const char*, int)> sendto_handler;
    
    // constructors
    Socket<UDP>(Inet<LinkLayer,IP4>&);
    Socket<UDP>(Inet<LinkLayer,IP4>&, port_t port);
    Socket<UDP>(const Socket<UDP>&) = delete;
    
    // functions
    inline void onRead(recvfrom_handler func)
    {
      on_read = func;
    }
    inline void onWrite(sendto_handler func)
    {
      on_send = func;
    }
    int sendto(addr_t destIP, port_t port, 
               const void* buffer, int length);
    int bcast(addr_t srcIP, port_t port, 
              const void* buffer, int length);
    void close();
    
    void join(multicast_group_addr&);
    void leave(multicast_group_addr&);
    
    // stuff
    addr_t local_addr() const
    {
      return stack.ip_addr();
    }
    port_t local_port() const
    {
      return l_port;
    }
    
  private:
    void packet_init(std::shared_ptr<PacketUDP>, addr_t, addr_t, port_t, uint16_t);
    int  internal_read(std::shared_ptr<PacketUDP>);
    int  internal_write(addr_t, addr_t, port_t, const uint8_t*, int);
    
    Inet<LinkLayer,IP4>& stack;
    port_t l_port;
    recvfrom_handler on_read = [](Socket<UDP>&, addr_t, port_t, const char*, int)->int{ return 0; };
    sendto_handler   on_send = [](Socket<UDP>&, addr_t, port_t, const char*, int)->int{ return 0; };
    
    bool reuse_addr;
    bool loopback; // true means multicast data is looped back to sender
    
    friend class UDP;
  };
}

#endif
