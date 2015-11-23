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
    using on_config = delegate<void(Stack&)>;
    
    DHClient() = delete;
    DHClient(DHClient&) = delete;
    DHClient(Stack&);
    
    Stack& stack;
		void negotiate(); // --> offer
    on_config onConfig;
    
	private:
		void offer(SocketUDP&, const char* data, int len);
		void request(SocketUDP&);   // --> acknowledge
    void acknowledge(const char* data, int len);
    
    uint32_t  xid;
    IP4::addr ipaddr, netmask, router;
    uint32_t  lease_time;
	};
	
  inline DHClient::DHClient(Stack& inet)
    : stack(inet)  {}
}

#endif
