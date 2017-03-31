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
#include <net/inet4>
#include <timers>
#include <net/http/request.hpp>
#include <net/http/response.hpp>

using namespace std::chrono;

std::string HTML_RESPONSE()
{
  const int color = rand();

  // Generate some HTML
  std::stringstream stream;
  stream << "<!DOCTYPE html><html><head>"
         << "<link href='https://fonts.googleapis.com/css?family=Ubuntu:500,300'"
         << " rel='stylesheet' type='text/css'>"
         << "<title>IncludeOS Demo Service</title></head><body>"
         << "<h1 style='color: #" << std::hex << ((color >> 8) | 0x020202)
         << "; font-family: \"Arial\", sans-serif'>"
         << "Include<span style='font-weight: lighter'>OS</span></h1>"
         << "<h2>The C++ Unikernel</h2>"
         << "<p>You have successfully booted an IncludeOS TCP service with simple http. "
         << "For a more sophisticated example, take a look at "
         << "<a href='https://github.com/hioa-cs/IncludeOS/tree/master/examples/acorn'>Acorn</a>.</p>"
         << "<footer><hr/>&copy; 2017 IncludeOS </footer></body></html>";

  return stream.str();
}

http::Response handle_request(const http::Request& req)
{
  printf("<Service> Request:\n%s\n", req.to_string().c_str());

  http::Response res;

  auto& header = res.header();

  header.set_field(http::header::Server, "IncludeOS/0.10");

  // GET /
  if(req.method() == http::GET && req.uri().to_string() == "/")
  {
    // add HTML response
    res.add_body(HTML_RESPONSE());

    // set Content type and length
    header.set_field(http::header::Content_Type, "text/html; charset=UTF-8");
    header.set_field(http::header::Content_Length, std::to_string(res.body().to_string().size()));
  }
  else
  {
    // Generate 404 response
    res.set_status_code(http::Not_Found);
  }

  header.set_field(http::header::Connection, "close");

  return res;
}

void Service::start(const std::string&)
{
  // DHCP on interface 0
  printf("*** Waiting up to 10 sec. for DHCP... ***\n");
  auto& inet = net::Inet4::ifconfig(5.0, [](bool timeout) {
    if (timeout) {
      printf("*** Falling back to static network config ***\n");
      // static IP in case DHCP fails
      net::Inet4::stack().network_config(
        { 10,0,0,42 },     // IP
        { 255,255,255,0 }, // Netmask
        { 10,0,0,1 },      // Gateway
        { 10,0,0,1 });     // DNS
    }
  });
  // Print some useful netstats every 30 secs
  Timers::periodic(5s, 30s,
  [&inet] (uint32_t) {
    printf("<Service> TCP STATUS:\n%s\n", inet.tcp().status().c_str());
  });

  // Set up a TCP server on port 80
  auto& server = inet.tcp().listen(80);

  // Add a TCP connection handler - here a hardcoded HTTP-service
  server.on_connect(
  [] (net::tcp::Connection_ptr conn) {
    printf("<Service> @on_connect: Connection %s successfully established.\n",
      conn->remote().to_string().c_str());
    // read async with a buffer size of 1024 bytes
    // define what to do when data is read
    conn->on_read(1024,
    [conn] (net::tcp::buffer_t buf, size_t n)
    {
      printf("<Service> @on_read: %u bytes received.\n", n);
      try
      {
        std::string data{(const char*)buf.get(), n};
        // try to parse the request
        http::Request req{data};

        // handle the request, getting a matching response
        auto res = handle_request(req);

        printf("<Service> Responding with %u %s.\n",
          res.status_code(), http::code_description(res.status_code()).to_string().c_str());

        conn->write(res);
      }
      catch(const std::exception& e)
      {
        printf("<Service> Unable to parse request:\n%s\n", e.what());
      }
    });
    conn->on_write([](size_t written) {
      printf("<Service> @on_write: %u bytes written.\n", written);
    });
  });

  printf("*** Basic demo service started ***\n");
}
