#ifndef NET_DHCP_DH4CLIENT_HPP
#define NET_DHCP_DH4CLIENT_HPP

#define DEBUG
#include "../packet.hpp"

#include <debug>
#include <info>

namespace net
{
  class SocketUDP;
  
  template <typename LINK, typename IPV>
  class Inet;
  
  class DHClient
  {
  public:
    using Stack = Inet<LinkLayer, IP4>;
    using On_config = delegate<void(Stack&)>;
    
    DHClient() = delete;
    DHClient(DHClient&) = delete;
    DHClient(Stack&);
    
    Stack& stack;
    void negotiate(); // --> offer
    inline void on_config(On_config handler){
      config_handler = handler;
    }
    
  private:
    void offer(SocketUDP&, const char* data, int len);
    void request(SocketUDP&);   // --> acknowledge
    void acknowledge(const char* data, int len);
    
    uint32_t  xid;
    IP4::addr ipaddr, netmask, router, dns_server;
    uint32_t  lease_time;
    On_config config_handler = [](Stack&){ INFO("DHCPv4::On_config","Config complete"); };
  };
	
  inline DHClient::DHClient(Stack& inet)
    : stack(inet)  {}
}

#endif
