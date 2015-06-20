#include <os>
#include <iostream>
#include <net/inet>
#include <net/class_ip6.hpp>

using namespace std;

#define SERVICE_PORT 5555
net::Inet* network;
/*
int listener(std::shared_ptr<net::Packet>& pckt)
{
  cout << "<DNS SERVER> got packet..." << endl;
  using namespace net;
  
  UDP::full_header& udp = *(UDP::full_header*) pckt->buffer();
  //DNS::header& hdr      = *(DNS::header*) (pckt->buffer() + sizeof(UDP::full_header));
  
  /// create response ///
  int packetlen = 0;
  
  /// send response back to client ///
  
  // set source & return address
  udp.udp_hdr.dport = udp.udp_hdr.sport;
  udp.udp_hdr.sport = htons(SERVICE_PORT);
  udp.udp_hdr.length = htons(sizeof(UDP::udp_header) + packetlen);
  
  // Populate outgoing IP header
  udp.ip_hdr.daddr = udp.ip_hdr.saddr;
  udp.ip_hdr.saddr = network->ip4(ETH0);
  udp.ip_hdr.protocol = IP4::IP4_UDP;
  
  // packet length (??)
  int res = pckt->set_len(sizeof(UDP::full_header) + packetlen); 
  if(!res)
    cout << "<DNS_SERVER> ERROR setting packet length failed" << endl;
  std::cout << "Returning " << packetlen << "b to " << udp.ip_hdr.daddr.str() << std::endl;  
  std::cout << "Full packet size: " << pckt->len() << endl;
  // return packet (as DNS response)
  network->udp_send(pckt);
  
  return 0;
}*/

void Service::start()
{
	std::cout << "*** Service is up - with OS Included! ***" << std::endl;
	
  using namespace net;
  auto& mac = Dev::eth(ETH0).mac();
  
  
  
  Inet::ifconfig(ETH0, // Interface
      {mac.part[2],mac.part[3],mac.part[4],mac.part[5]}, // IP
      {255,255,0,0}, // Netmask
      IP6::addr(255, 255) ); // IPv6
  
	//network = Inet::up();
	/*
  std::cout << "Starting UDP server on IP " 
			<< network->ip4(ETH0).str() << std::endl;
  */
	
	std::cout << "Service out!" << std::endl;
}
