#define DEBUG
#include <os>
#include <net/class_inet.hpp>

using namespace net;

Inet* Inet::instance = 0;

std::map<uint16_t,IP4::addr> Inet::_ip4_list;
std::map<uint16_t,IP4::addr> Inet::_netmask_list;
std::map<uint16_t,Ethernet*> Inet::_ethernet_list;
std::map<uint16_t,Arp*> Inet::_arp_list;

void Inet::ifconfig(netdev i, IP4::addr ip, IP4::addr netmask){  
  _ip4_list[i] = ip;
  _netmask_list[i] = netmask;
  debug("<Inet> I now have %li IP's\n", _ip4_list.size());
};
