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
      net::IP6::addr(1, 2, 3, 4, 5, 6, 7, 8));
  
  net::Inet* inet = net::Inet::up();
  
  printf("Service IP address: %s\n", net::Inet::ip4(net::ETH0).str().c_str());
  
  static const int UDP_PORT = 64;
  inet->udp6_listen(UDP_PORT,
    [] (std::shared_ptr<net::PacketUDP6>& pckt)
    {
      printf("Received UDP6 packet from %s to my listener on port %d\n",
          pckt->src().to_string().c_str(), pckt->dst_port());
      
      std::string data((const char*) pckt->data(), pckt->data_length());
      
      printf("Contents (len=%d):\n%s\n", pckt->data_length(), data.c_str());
      return -1;
    }
  );
  
}
