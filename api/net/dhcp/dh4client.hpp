#ifndef NET_DHCP_DH4CLIENT_HPP
#define NET_DHCP_DH4CLIENT_HPP

#define DEBUG
#include "../packet.hpp"
#include "../inet4.hpp"
#include <debug>

namespace net
{
	class DHClient
	{
	public:
		using Stack = Inet<LinkLayer, IP4>;
    
    DHClient(Stack&);
		void negotiate(); // --> offer
    
	private:
		int  offer(Packet_ptr packet);
		void request();   // --> acknowledge
    int  acknowledge(Packet_ptr packet);
    
    Stack& stack;
	};
	
  DHClient::DHClient(Stack& inet)
    : stack(inet)
  {
    printf("DHClient requesting IP4 address");
    negotiate();
  }
}

#endif
