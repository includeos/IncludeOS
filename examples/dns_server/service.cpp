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
	
  /// www.google.com ///
  std::vector<IP4::addr> mapping1;
  mapping1.push_back( { 213, 155, 151, 187 } );
  mapping1.push_back( { 213, 155, 151, 185 } );
  mapping1.push_back( { 213, 155, 151, 180 } );
  mapping1.push_back( { 213, 155, 151, 183 } );
  mapping1.push_back( { 213, 155, 151, 186 } );
  mapping1.push_back( { 213, 155, 151, 184 } );
  mapping1.push_back( { 213, 155, 151, 181 } );
  mapping1.push_back( { 213, 155, 151, 182 } );
  
  myDnsServer.addMapping("www.google.com.", mapping1);
  ///               ///
  
	myDnsServer.start(inet);
	std::cout << "<DNS SERVER> Listening on UDP port 53" << std::endl;
	
	std::cout << "Service out!" << std::endl;
}
