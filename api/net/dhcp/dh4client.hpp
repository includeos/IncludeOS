#ifndef NET_DHCP_DH4CLIENT_HPP
#define NET_DHCP_DH4CLIENT_HPP

#define DEBUG
#include "../packet.hpp"
#include "../inet4.hpp"
#include <debug>

namespace net
{
	class SocketUDP;
  
  class DHClient
	{
	public:
		using Stack = Inet<LinkLayer, IP4>;
    
    DHClient(Stack&);
		void negotiate(); // --> offer
    
	private:
		void offer(SocketUDP&, const char* data, int len);
		void request();   // --> acknowledge
    void acknowledge(Packet_ptr packet);
    
    uint32_t xid;
    uint32_t ipaddr, netmask, router;
    uint32_t server;
    
    Stack& stack;
	};
	
  inline DHClient::DHClient(Stack& inet)
    : stack(inet)
  {
    printf("DHClient requesting IP4 address\n");
    negotiate();
  }
}

#endif
