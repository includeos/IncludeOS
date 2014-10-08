#include <os>
#include <net/class_ip4.hpp>



void IP4::handler(uint8_t* data, int len){
  printf("<IP4 handler> got the data, but I'm clueless: DROP! \n");
};

IP4::IP4(Ethernet& eth){
  // Assign myself as ARP handler for eth
  eth.set_ip4_handler(delegate<void(uint8_t*,int)>::from<IP4,
               &IP4::handler>(this));

}
