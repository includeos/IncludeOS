#include <os>
#include <net/inet4>
#include <math.h>
#include <iostream>
#include <sstream>

//#include <thread> => <thread> is not supported on this single threaded system

using namespace std::chrono;

void Service::start()
{

  // Wonder when these are used...?
  std::set_terminate([](){ printf("CUSTOM TERMINATE Handler \n"); });
  std::set_new_handler([](){ printf("CUSTOM NEW Handler \n"); });
  
  // TODO: find some implementation for long double, or not... or use double
  //auto sine = sinl(42);
    
    // Assign an IP-address, using Hårek-mapping :-)
  auto& eth0 = Dev::eth<0,VirtioNet>();
  auto& mac = eth0.mac(); 
  
  net::Inet4<VirtioNet>::ifconfig(net::ETH0, 
		       {{ mac.part[2],mac.part[3],mac.part[4],mac.part[5] }},
		       {{ 255,255,0,0 }} );
  
  // Bring up the interface
  net::Inet4<VirtioNet>& inet = net::Inet4<VirtioNet>::up(eth0);
  printf("Service IP address: %s \n", net::Inet4<VirtioNet>::ip4(net::ETH0).str().c_str());
  
  // Set up a server on port 80
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


  uint64_t my_llu = 42;
  printf("BUG? My llu is: %llu, and 42 == %i \n",my_llu, 42);
  
  auto& bufstore = net::Packet::bufstore();
  net::Packet pckt(net::Packet::DOWNSTREAM);
  printf("Service made a packet! Size: %i \n", pckt.len());
  
  printf("*** TEST SERVICE v0.6.3-proto STARTED *** \n");
}
