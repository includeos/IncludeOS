#include <os>
#include <net/class_ip6.hpp>



void IP6::handler(uint8_t* data, int len){
  printf("<IP6 handler> got the data, but I'm clueless: DROP! \n");
};

IP6::IP6(Ethernet& eth)
  : _eth(eth)
{
  // Assign myself as ARP handler for eth
  eth.set_ip6_handler(delegate<void(uint8_t*,int)>::from<IP6,
               &IP6::handler>(this));

}
