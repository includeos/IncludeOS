//#define DEBUG // Allow debugging
#include <os>
#include <net/class_ip6.hpp>



int IP6::bottom(uint8_t* UNUSED(data), int UNUSED(len)){
  debug("<IP6 handler> got the data, but I'm clueless: DROP! \n");
  return -1;
};

