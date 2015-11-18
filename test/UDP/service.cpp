//#define DEBUG // Debug supression

#include <os>
#include <list>
#include <net/inet4>
#include <vector>

using namespace std;
using namespace net;

uint8_t* buf = 0;
int bufsize = 0;
uint8_t* prev_data = 0;


void Service::start()
{
  

// Assign an IP-address, using Hårek-mapping :-)
  auto& eth0 = Dev::eth<0,VirtioNet>();
  auto& mac = eth0.mac(); 
  
  auto& inet = *new net::Inet4<VirtioNet>(eth0, // Device
    {{ mac.part[2],mac.part[3],mac.part[4],mac.part[5] }}, // IP
    {{ 255,255,0,0 }} );  // Netmask
  
  printf("Service IP address: %s \n", inet.ip_addr().str().c_str());
  
  // UDP
  UDP::port port = 4242;
  auto& sock = inet.udp().bind(port);
  
  sock.onRead([](SocketUDP& conn, UDP::addr_t addr, UDP::port port, const std::string& str)->int{
      printf("Getting UDP data from %s : %i: %s \n", 
	     addr.str().c_str(), port, str.c_str());
      return 0;
    });
  
  printf("UDP server listening to port %i \n",port);
  
}
