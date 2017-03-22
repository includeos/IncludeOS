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
#include <timers>
#include <profile>
#include <string>

using namespace std::chrono;

std::string create_html_response(const std::string& message)
{
  return "HTTP/1.1 200 OK\n"
         "Date: Mon, 01 Jan 1970 00:00:01 GMT\n"
         "Server: IncludeOS prototype 4.0\n"
         "Last-Modified: Wed, 08 Jan 2003 23:11:55 GMT\n"
         "Content-Type: text/html; charset=UTF-8\n"
         "Content-Length: " + std::to_string(message.size()) + "\n"
         "Accept-Ranges: bytes\n"
         "Connection: close\n"
         "\n"
         + message;
}

void Service::start(const std::string&)
{
  // DHCP on interface 0
  auto& inet = net::Inet4::ifconfig<0>(10.0);

  // static IP in case DHCP fails
  net::Inet4::ifconfig(
    { 10,0,0,42 },     // IP
    { 255,255,255,0 }, // Netmask
    { 10,0,0,1 },      // Gateway
    { 10,0,0,1 });     // DNS

  // Set up a TCP server on port 80
  auto& server = inet.tcp().listen(80);

  server.on_connect([](auto conn)
  {
    conn->on_read(1024, [conn](net::tcp::buffer_t buf, size_t n)
    {
      auto data = std::string(reinterpret_cast<char*>(buf.get()), n);
      if (data.find("GET /profile ") != std::string::npos)
      {
        auto profile_statistics = ScopedProfiler::get_statistics();
        auto response = create_html_response(profile_statistics + "\n");
        conn->write(response.data(), response.size());
      }
      else
      {
        auto response = create_html_response("Hello\n");
        conn->write(response.data(), response.size());
      }
    });

    conn->on_disconnect([](auto conn, auto reason)
    {
      (void)reason;  // Not used
      conn->close();
    });
  });

  printf("*** TEST SERVICE STARTED ***\n");
}
