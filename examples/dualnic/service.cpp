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

#include <cmath>   // rand()
#include <sstream>

#include <os>
#include <net/inet4>
#include <net/dhcp/dh4client.hpp>

using namespace std::chrono;

std::string HTML_RESPONSE();

void create_server(net::TCP::Connection& conn)
{
  // Add a TCP connection handler - here a hardcoded HTTP-service
  conn.onAccept(
  [] (auto conn) -> bool
  {
    printf("<Service> @onAccept - Connection attempt from: %s \n",
           conn->to_string().c_str());
    return true; // allow all connections

  }).onConnect(
  [] (auto conn) {
    printf("<Service> @onConnect - Connection successfully established.\n");
    // read async with a buffer size of 1024 bytes
    // define what to do when data is read
    conn->read(1024, [conn](net::TCP::buffer_t buf, size_t n) {
        // create string from buffer
        std::string data { (char*)buf.get(), n };
        printf("<Service> @read:\n%s\n", data.c_str());

        // create response
        std::string response = HTML_RESPONSE();
        // write the data from the string with the strings size
        conn->write(response.data(), response.size(), [](size_t n) {
            printf("<Service> @write: %u bytes written\n", n);
          });
      });

  }).onDisconnect(
  [] (auto conn, auto reason) {
      printf("<Service> @onDisconnect - Reason: %s \n", reason.to_string().c_str());
      conn->close();
  });
}

void Service::start() {
  srand(OS::cycles_since_boot());

  // Two IP-stack objects

  // Stack with network interface (eth0) driven by VirtioNet
  // DNS address defaults to 8.8.8.8
  // Static IP configuration, until we (possibly) get DHCP
  // @note : Mostly to get a robust demo service that works with and without DHCP
  static auto inet1 = net::new_ipv4_stack<>({ 10,0,0,42 },      // IP
                                            { 255,255,255,0 },  // Netmask
                                            { 10,0,0,1 });      // Gateway

  // Assign a driver (VirtioNet) to network interface (eth1)
  // @note: We could determine the appropirate driver dynamically, but then we'd
  // have to include all the drivers into the image, which we want to avoid.
  static auto inet2 = net::new_ipv4_stack<1,VirtioNet>({ 20,0,0,42 },      // IP
                                                       { 255,255,255,0 },  // Netmask
                                                       { 20,0,0,1 });      // Gateway
  // Set up a TCP server on port 80
  auto& server1 = inet1->tcp().bind(80);
  create_server(server1);

  auto& server2 = inet2->tcp().bind(80);
  create_server(server2);

  // Print some useful netstats every 30 secs
  hw::PIT::instance().onRepeatedTimeout(30s, [] {
    printf("<INET_1> TCP STATUS:\n\t%s\n", inet1->tcp().status().c_str());
    printf("<INET_2> TCP STATUS:\n\t%s\n", inet2->tcp().status().c_str());
  });

  printf("*** TEST SERVICE STARTED *** \n");
}


std::string HTML_RESPONSE() {
  const int color = rand();

  /* HTML Fonts */
  const std::string ubuntu_medium = "font-family: \'Ubuntu\', sans-serif; font-weight: 500; ";
  const std::string ubuntu_normal = "font-family: \'Ubuntu\', sans-serif; font-weight: 400; ";
  const std::string ubuntu_light  = "font-family: \'Ubuntu\', sans-serif; font-weight: 300; ";

  /* HTML */
  std::stringstream stream;
  stream << "<!DOCTYPE html><html><head>"
         << "<link href='https://fonts.googleapis.com/css?family=Ubuntu:500,300' rel='stylesheet' type='text/css'>"
         << "</head><body>"
         << "<h1 style='color: #" << std::hex << (color >> 8) << "'>"
         <<  "<span style='"+ubuntu_medium+"'>Include</span><span style='"+ubuntu_light+"'>OS</span></h1>"
         <<  "<h2>Now speaks TCP!</h2>"
         // .... generate more dynamic content
         << "<p>  ...and can improvise http. With limitations of course, but it's been easier than expected so far </p>"
         << "<footer><hr/> &copy; 2015, Oslo and Akershus University College of Applied Sciences </footer>"
         << "</body></html>";

  const std::string html = stream.str();

  std::string header
  {
    "HTTP/1.1 200 OK\n"
    "Date: Mon, 01 Jan 1970 00:00:01 GMT\n"
    "Server: IncludeOS prototype 4.0\n"
    "Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\n"
    "Content-Type: text/html; charset=UTF-8\n"
    "Content-Length: "+std::to_string(html.size())+"\n"
    "Accept-Ranges: bytes\n"
    "Connection: close\n\n"
  };

  return header + html;
}
