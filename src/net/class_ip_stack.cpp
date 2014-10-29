#define DEBUG
#include <os>
#include <net/class_ip_stack.hpp>

using namespace net;

IP_stack* IP_stack::instance = 0;

std::map<uint16_t,IP4::addr> IP_stack::ip4_list;
std::map<uint16_t,Ethernet*> IP_stack::ethernet_list;
std::map<uint16_t,Arp*> IP_stack::arp_list;

void IP_stack::ifconfig(netdev i, IP4::addr ip, IP4::addr UNUSED(netmask)){  
  ip4_list[i]=ip;
  debug("<IP Stack> I now have %li IP's",ip4_list.size());
};
