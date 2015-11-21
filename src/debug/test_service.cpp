#include <os>
#include <net/inet4>
#include <math.h>
#include <iostream>
#include <sstream>
#include <net/dhcp/dh4client.hpp>

using namespace std::chrono;

void Service::start() {
  
  // Assign an IP-address, using Hårek-mapping :-)
  auto& eth0 = Dev::eth<0,VirtioNet>();
  auto& mac = eth0.mac(); 
  
  auto& inet = *new net::Inet4<VirtioNet>(eth0, // Device
    {{ mac.part[2],mac.part[3],mac.part[4],mac.part[5] }}, // IP
    {{ 255,255,0,0 }} );  // Netmask
  
  // negotiate with terrorists
  net::DHClient dhclient(inet);
  
  printf("Size of IP-stack: %i bytes \n",sizeof(inet));
  printf("Service IP address: %s \n", inet.ip_addr().str().c_str());
  
  // Set up a TCP server on port 80
  net::TCP::Socket& sock =  inet.tcp().bind(80);
  
  printf("SERVICE: %i open ports in TCP @ %p \n",
      inet.tcp().openPorts(), &(inet.tcp()));   

  srand(OS::cycles_since_boot());
  
  
  
  sock.onConnect([](net::TCP::Socket& conn){
      printf("SERVICE got data: %s \n",conn.read(1024).c_str());
      
      int color = rand();
      std::stringstream stream;
      
      std::string ubuntu_medium  = "font-family: \'Ubuntu\', sans-serif; font-weight: 500; ";
      std::string ubuntu_normal  = "font-family: \'Ubuntu\', sans-serif; font-weight: 400; ";
      std::string ubuntu_light  = "font-family: \'Ubuntu\', sans-serif; font-weight: 300; ";
      
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
  
  
  /** TEST ARP-resolution 
      @todo move to separate location
  auto pckt = std::static_pointer_cast<net::PacketIP4>(inet.createPacket(50));
  pckt->init();
  pckt->next_hop({{ 10,0,0,1 }});  
  pckt->set_src(inet.ip_addr());
  pckt->set_dst(pckt->next_hop());
  pckt->set_protocol(net::IP4::IP4_UDP);
  inet.ip_obj().transmit(pckt);
  */
  
  printf("*** TEST SERVICE STARTED *** \n");    

}
