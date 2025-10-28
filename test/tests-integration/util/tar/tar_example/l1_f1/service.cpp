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

#include <cmath> // rand()
#include <sstream>

#include <os>
#include <net/inet>
#include <timers>

using namespace std::chrono;

std::string HTML_RESPONSE()
{
  const int color = rand();

  /* HTML Fonts */
  std::string ubuntu_medium  = "font-family: \'Ubuntu\', sans-serif; font-weight: 500; ";
  std::string ubuntu_normal  = "font-family: \'Ubuntu\', sans-serif; font-weight: 400; ";
  std::string ubuntu_light   = "font-family: \'Ubuntu\', sans-serif; font-weight: 300; ";

  /* HTML */
  std::stringstream stream;
  stream << "<!DOCTYPE html><html><head>"
         << "<link href='https://fonts.googleapis.com/css?family=Ubuntu:500,300' rel='stylesheet' type='text/css'>"
         << "</head><body>"
         << "<h1 style='color: #" << std::hex << (color >> 8) << "'>"
         << "<span style='"+ubuntu_medium+"'>Include</span><span style='"+ubuntu_light+"'>OS</span></h1>"
         <<  "<h2>Now speaks TCP!</h2>"
         // .... generate more dynamic content
         << "<p>This is improvised http, but proper stuff is in the works.</p>"
         << "<footer><hr/>&copy; 2016, IncludeOS AS @ 60&deg; north</footer>"
         << "</body></html>";

  const std::string html = stream.str();

  const std::string header
  {
    "HTTP/1.1 200 OK\n"
    "Date: Mon, 01 Jan 1970 00:00:01 GMT\n"
    "Server: IncludeOS prototype 4.0\n"
    "Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\n"
    "Content-Type: text/html; charset=UTF-8\n"
    "Content-Length: "+std::to_string(html.size())+'\n'+
    "Accept-Ranges: bytes\n"
    "Connection: close\n\n"
  };

  return header + html;
}

const std::string NOT_FOUND = "HTTP/1.1 404 Not Found\nConnection: close\n\n";

void Service::start(const std::string&)
{
  // DHCP on interface 0
  auto& inet = net::Inet::ifconfig(10.0);
  // static IP in case DHCP fails
  net::Inet::ifconfig(
    { 10,0,0,42 },     // IP
    { 255,255,255,0 }, // Netmask
    { 10,0,0,1 },      // Gateway
    { 10,0,0,1 });     // DNS

  // Print some useful netstats every 30 secs
  Timers::periodic(5s, 30s,
  [&inet] (uint32_t) {
    printf("<Service> TCP STATUS:\n%s\n", inet.tcp().status().c_str());
  });

  // Set up a TCP server on port 80
  auto& server = inet.tcp().bind(80);

  // Add a TCP connection handler - here a hardcoded HTTP-service
  server.on_accept(
  [] (auto socket) -> bool {
    printf("<Service> @onAccept - Connection attempt from: %s\n",
           socket.to_string().c_str());
    return true; // allow all connections
  })
  .on_connect(
  [] (auto conn) {
    printf("<Service> @onConnect - Connection successfully established.\n");
    // read async with a buffer size of 1024 bytes
    // define what to do when data is read
    conn->on_read(1024,
    [conn] (net::tcp::buffer_t buf, size_t n) {
      // create string from buffer
      std::string data { (char*)buf.get(), n };
      printf("<Service> @read:\n%s\n", data.c_str());

      if (data.find("GET / ") != std::string::npos)
      {
        // create response
        std::string response = HTML_RESPONSE();
        // write the data from the string with the strings size
        conn->write(response.data(), response.size(), [](size_t n) {
            printf("<Service> @write: %u bytes written\n", n);
          });
      }
      else {
        conn->write(NOT_FOUND.data(), NOT_FOUND.size());
      }
    });
    conn->on_disconnect(
    [] (auto conn, auto reason) {
        printf("<Service> @onDisconnect - Reason: %s\n", reason.to_string().c_str());
        conn->close();
    })
    .on_error(
    [] (auto err) {
      printf("<Service> @onError - %s\n", err.what());
    });
  });

  printf("*** TEST SERVICE STARTED ***\n");
}
