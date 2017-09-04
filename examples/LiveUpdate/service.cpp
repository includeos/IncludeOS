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
#include "liu.hpp"

using namespace std::chrono;
http::Response handle_request(const http::Request& req);

void Service::start()
{
  // Get the first IP stack
  auto& inet = net::Super_stack::get<net::IP4>(0);

  // Print some useful netstats every 30 secs
  Timers::periodic(5s, 30s,
  [&inet] (uint32_t) {
    printf("<Service> TCP STATUS:\n%s\n", inet.tcp().status().c_str());
  });

  // Set up a TCP server on port 80
  auto& server = inet.tcp().listen(80);

  // Add a TCP connection handler - here a hardcoded HTTP-service
  server.on_connect(
  [] (auto conn)
  {
    printf("<Service> @on_connect: Connection %s successfully established.\n",
            conn->remote().to_string().c_str());
    // read async with a buffer size of 1024 bytes
    // define what to do when data is read
    conn->on_read(1024,
    [conn] (auto buf, size_t n)
    {
      printf("<Service> @on_read: %lu bytes received.\n", n);
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
  });

  if (liu::LiveUpdate::is_resumable() == false)
  {
    setup_liveupdate_server(inet, 666, nullptr);
  }
  else {
    printf("System live updated!\n");
  }
}
