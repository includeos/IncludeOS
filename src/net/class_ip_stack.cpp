#include <os>
#include <net/class_ip_stack.hpp>

using namespace net;

int IP_stack::ifconfig(netdev UNUSED(i), IP4::addr ip, IP4::addr UNUSED(netmask)){
  
  // Later, this needs to do a lookup to find the right ARP-object
  _arp.set_ip(ip);
  
  return 0;
  
};
