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

std::vector<std::string> BUFFER;
std::shared_ptr<net::TCP::Connection> RSH_PEER {nullptr};
void write_rsh(const char* data, size_t n) {
  //OS::rswrite('X');
  RSH_PEER->write(data, n);
}

void Service::start() {
  // Assign a driver (VirtioNet) to a network interface (eth0)
  // @note: We could determine the appropirate driver dynamically, but then we'd
  // have to include all the drivers into the image, which  we want to avoid.
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  
  // Bring up a network stack, attached to the nic
  // @note : No parameters after 'nic' means we'll use DHCP for IP config.
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);
  
  // Static IP configuration, until we (possibly) get DHCP
  // @note : Mostly to get a robust demo service that it works with and without DHCP
  inet->network_config( {{ 10,0,0,42 }},      // IP
			{{ 255,255,255,0 }},  // Netmask
			{{ 10,0,0,1 }},       // Gateway
			{{ 8,8,8,8 }} );      // DNS
  
  srand(OS::cycles_since_boot());

  printf("Size of IP-stack: %i b \n",sizeof(inet));
  printf("Service IP address: %s \n", inet->ip_addr().str().c_str());

  // buffer up all output until connection is established
  /*OS::set_rsprint([](const char* data, size_t n){
    BUFFER.push_back({data, n});
  });*/
    
  auto& rsh = inet->tcp().bind(22);

  

  rsh.onAccept([](auto conn)->bool {
      return true;
  }).onConnect([](auto conn) {
    // change print to write directly to connection.
    printf("Established remote shell connection, changing rsprint... \n");
    RSH_PEER = conn;
    OS::set_rsprint(&write_rsh);
  });



  // Set up a TCP server on port 80
  auto& server = inet->tcp().bind(80);
  
  printf("<Service> Connection: %s \n", server.to_string().c_str());
  // Add a TCP connection handler - here a hardcoded HTTP-service
  server.onAccept([](auto conn)->bool {
      printf("<Service> @onAccept - Connection attempt from: %s \n", conn->remote().to_string().c_str());
      printf("<Service> Status: %s \n", conn->to_string().c_str());

      bool allowed_connection_expression = true; // accepts all connections.
      
      return allowed_connection_expression;

  }).onConnect([](auto conn) {
      printf("<Service> @onConnect - Connection successfully established. \n");
      printf("<Service> TCP STATUS:\n%s \n", conn->host().status().c_str());

  }).onReceive([](auto conn, bool push) {
      std::string data = conn->read(1024);
      printf("<Service> @onData - PUSH: %d, Data read: \n %s \n", push, data.c_str());
      printf("<Service> Status: %s \n", conn->to_string().c_str());
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
      std::string header="HTTP/1.1 200 OK \n "        \
  "Date: Mon, 01 Jan 1970 00:00:01 GMT \n"      \
  "Server: IncludeOS prototype 4.0 \n"        \
  "Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT \n"   \
  "Content-Type: text/html; charset=UTF-8 \n"     \
  "Content-Length: "+std::to_string(html.size())+"\n"   \
  "Accept-Ranges: bytes\n"          \
  "Connection: close\n\n";
      
      std::string output{header + html};
      conn->write(output.data(), output.size());

  }).onDisconnect([](auto conn, std::string msg) {
      printf("<Service> @onDisconnect - Reason: %s \n", msg.c_str());
      printf("<Service> TCP STATUS:\n%s \n", conn->host().status().c_str());

  });

  // Set up a Active conneciton
  /*net::TCP::Connection& active_con = inet->tcp().connect({{23,235,43,133}});

  active_con.onConnect([](net::TCP::Connection& conn) {
      std::string request{"GET / HTTP/1.1 \n"};
      conn.write(request.data(), request.size());
  }).onData([](net::TCP::Connection& conn, bool PUSH) {

  }).onDisconnect([](net::TCP::Connection& conn, std::string msg) {

  });*/

  printf("*** TEST SERVICE STARTED *** \n");

}
