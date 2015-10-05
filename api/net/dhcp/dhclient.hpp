#ifndef NET_DHCLIENT_HPP
#define NET_DHCLIENT_HPP

namespace includeOS
{
namespace net
{
	class DHCP4;
	class DHCP6;
	
	class dhclient_base
	{
	public:
		void negotiate() = 0;
	}
	
	template <class DHCP4>
	class dhclient
		: public dhclient_base
	{
	public:
		dhclient();
		
		void negotiate();
		
	private:
		DHCP4 dhcp;
	};
	
}
}

#endif
