#ifdef __linux__
#include "linux_dns.hpp"

int main(void)
{
	LinuxDNS dns;
	// set nameserver
	dns.set_ns("8.8.8.8");
	
	// dig up some dirt
	if (dns.request("www.google.com"))
		dns.print();
	if (dns.request("www.fwsnet.net"))
		dns.print();
	if (dns.request("www.vg.no"))
		dns.print();
	
	return 0;
}

#elif defined(__includeOS__)
#include <os>
#include "include_dns.hpp"
#include <iostream>

using namespace net;
Inet* network;
PacketStore packetStore(50, 1500);

void Service::start()
{
  IP4::addr nameserver {8, 8, 8, 8};
  //IP4::addr nameserver {10, 0, 0, 10};
  
  std::cout << "addr: " << nameserver.str() << std::endl;
  
  Inet::ifconfig(net::ETH0,
                 { 10,  0, 0, 2},
                 {255,255, 0, 0});
	
	network = Inet::up();
	
	IncludeDNS dns;
	// set nameserver
  dns.set_ns(nameserver.whole);
	
	// dig up some dirt
  dns.request("www.google.com");
  //dns.request("www.vg.no");
  //dns.request("gonzo-All-series");
}

#endif
