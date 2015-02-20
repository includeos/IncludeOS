#include <os>
#include <class_dev.hpp>
#include <assert.h>
#include <net/inet>
#include <list>
#include <memory>

// Locals
#include "dns_server.hpp"

DNS_server myDnsServer;

void Service::start()
{
	std::cout << "*** Service is up - with OS Included! ***" << std::endl;
	std::cout << "Starting DNS prototype\n";
	
	using namespace net;
        
	auto& mac = Dev::eth(0).mac();
  Inet::ifconfig(net::ETH0, // Interface
      {mac.part[2],mac.part[3],mac.part[4],mac.part[5]}, // IP
      {255,255,0,0}); // Netmask
  
	Inet* inet = Inet::up();
	std::cout << "...Starting UDP server on IP " 
			<< inet->ip4(net::ETH0).str() << std::endl;
        // Populate registry
        DNS_server::init();
	myDnsServer.start(inet);
	std::cout << "<DNS SERVER> Listening on UDP port 53" << std::endl;
	
	std::cout << "Service out!" << std::endl;
}
