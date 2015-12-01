// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
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

#include <os>
#include <net/inet4>
#include <math.h>
#include <iostream>
#include <sstream>
#include <net/dhcp/dh4client.hpp>

using namespace std::chrono;

// An IP-stack object
std::unique_ptr<net::Inet4<VirtioNet> > inet;

// our VGA output module
#include <kernel/vga.hpp>
ConsoleVGA vga;

void Service::start()
{
  // set secondary serial output to VGA console module
  OS::set_rsprint_secondary(
  [] (const char* string, size_t len)
  {
    vga.write(string, len);
  });
  
  // Assign a driver (VirtioNet) to a network interface (eth0)
  // @note: We could determine the appropirate driver dynamically, but then we'd
  // have to include all the drivers into the image, which  we want to avoid.
  Nic<VirtioNet>& eth0 = Dev::eth<0,VirtioNet>();
  
  // Bring up a network stack, attached to the nic
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);

  // Static IP configuration, until we (possibly) get DHCP
  // @note : Mostly to get a robust demo service that it works with and without DHCP
  inet->network_config( {{ 10,0,0,42 }},      // IP
			{{ 255,255,255,0 }},  // Netmask
			{{ 10,0,0,1 }},       // Gateway
			{{ 8,8,8,8 }} );      // DNS

  inet->link().set_packet_filter([](net::Packet_ptr pckt){      
      printf("Custom Ethernet Packet filter got %i bytes\n",pckt->size());
      return pckt;
    });
  
  // Set a custom packet filter for IP
  auto current_handler = inet->link().get_ip4_handler();
  inet->link().set_ip4_handler([current_handler](net::Packet_ptr pckt)->int{
      auto pckt4 = std::static_pointer_cast<net::PacketIP4>(pckt);
      printf("Custom IP-level Packet filter got %i bytes from %s \n",
	     pckt->size(), pckt4->src().str().c_str());
      return current_handler(pckt);
    });
  
  inet->tcp().connect({{ 10,0,0,1 }}, 4242, [](net::TCP::Socket& conn){
      printf("TCP Connected to .... some IP. Data: '%s'\n", conn.read(1024).c_str()); 
      conn.write("Hello!\n");
      conn.close();
  });
    
      // after DHCP we would like to do some networking
  inet->dhclient()->on_config(
  [] (net::DHClient::Stack& stack)
  {
    const std::string hostname = "includeos.org";
    printf("*** Resolving %s\n", hostname.c_str());
    
    // after configuring our device, we will be
    // resolving some hostname
    stack.resolve(hostname,
    [] (net::DNSClient::Stack& stack,
        const std::string& hostname,
        net::IP4::addr addr)
    {
      // the answer has come through,
      // verify that the hostname was resolved
      if (addr == net::IP4::INADDR_ANY)
      {
          printf("Failed to resolve %s!\n",
              hostname.c_str());
          return;
      }
      printf("*** Resolved %s to %s!\n",
          hostname.c_str(), addr.str().c_str());
      
      // we will be sending UDP data to the resolved IP-address
      const int port = 4444;
      // sending messages into the ether is important, for science
      std::string data = "Is anyone there?\n";
      
      auto& sock = stack.udp().bind();
      printf("Sending %u bytes of UDP data to %s:%d from local port %d\n",
          data.size(), addr.str().c_str(), port, sock.local_port());
      
      sock.sendto(addr, port, data.c_str(), data.size());
      
      printf("Done. You can Ctrl-A + X now.\n");
    });
  });
  
  printf("Size of IP-stack: %i bytes \n",sizeof(*inet));
  printf("Service IP address: %s \n", inet->ip_addr().str().c_str());
  
  // Set up a TCP server on port 80
  net::TCP::Socket& sock =  inet->tcp().bind(80);
  
  printf("SERVICE: %i open ports in TCP @ %p \n",
      inet->tcp().openPorts(), &(inet->tcp()));

  srand(OS::cycles_since_boot());
  
  sock.onAccept([](net::TCP::Socket& conn){
      printf("SERVICE got data: %s \n",conn.read(1024).c_str());
      
      int color = rand();
      std::stringstream stream;
 
      /* HTML Fonts */
      std::string ubuntu_medium  = "font-family: \'Ubuntu\', sans-serif; font-weight: 500; ";
      std::string ubuntu_normal  = "font-family: \'Ubuntu\', sans-serif; font-weight: 400; ";
      std::string ubuntu_light  = "font-family: \'Ubuntu\', sans-serif; font-weight: 300; ";
      
      /* HTML */
      stream << "<html><head>"
	     << "<link href='https://fonts.googleapis.com/css?family=Ubuntu:500,300' rel='stylesheet' type='text/css'>"
	     << "</head><body>"
	     << "<h1 style= \"color: " << "#" << std::hex << (color >> 8) << "\">"	
	     <<  "<span style=\""+ubuntu_medium+"\">Include</span><span style=\""+ubuntu_light+"\">OS</span> </h1>"
	     <<  "<h2>Now speaks TCP!</h2>"
	// .... generate more dynamic content 
	     << "<p>  ...and can improvise http. With limitations of course, but it's been easier than expected so far </p>"
	     << "<footer><hr /> &copy; 2015, Oslo and Akershus University College of Applied Sciences </footer>"
	     << "</body></html>\n";
      
      /* HTTP-header */
      std::string html = stream.str();
      std::string header="HTTP/1.1 200 OK \n "				\
	"Date: Mon, 01 Jan 1970 00:00:01 GMT \n"			\
	"Server: IncludeOS prototype 4.0 \n"				\
	"Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT \n"		\
	"Content-Type: text/html; charset=UTF-8 \n"			\
	"Content-Length: "+std::to_string(html.size())+"\n"		\
	"Accept-Ranges: bytes\n"					\
	"Connection: close\n\n";
      
      conn.write(header);
      conn.write(html);
      
      // We don't have to actively close when the http-header says "Connection: close"
      //conn.close();
      
    });
  
  printf("*** TEST SERVICE STARTED *** \n");    

}
