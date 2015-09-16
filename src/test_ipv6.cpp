#include <os>
#include <stdio.h>
#include <net/inet>

void Service::start()
{
  auto& mac = Dev::eth(0).mac();
  
  net::Inet::ifconfig(
      net::ETH0, 
      {{ mac.part[2],mac.part[3],mac.part[4],mac.part[5] }}, 
      {{255, 255, 255, 0}}, 
      {0, 0});
  
  net::Inet* inet = net::Inet::up();
  
  printf("Service IP address: %s\n", net::Inet::ip4(net::ETH0).str().c_str());
  
}
