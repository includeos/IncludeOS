// Part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and  Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// 
//     http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

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
