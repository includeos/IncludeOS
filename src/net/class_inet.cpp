#define DEBUG
#include <os>
#include <net/class_inet.hpp>

using namespace net;

Inet* Inet::instance = 0;

std::map<uint16_t,IP4::addr> Inet::ip4_list;
std::map<uint16_t,Ethernet*> Inet::ethernet_list;
std::map<uint16_t,Arp*> Inet::arp_list;

void Inet::ifconfig(netdev i, IP4::addr ip, IP4::addr UNUSED(netmask)){  
  ip4_list[i]=ip;
  debug("<Inet> I now have %li IP's",ip4_list.size());
};
