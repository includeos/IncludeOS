//#define DEBUG // Allow debugging
#include <os>
#include <net/class_ip6.hpp>

namespace net
{
	int IP6::bottom(std::shared_ptr<Packet>& UNUSED(pckt))
	{
		debug("<IP6 handler> got the data, but I'm clueless: DROP! \n");
		return -1;
	};
}
