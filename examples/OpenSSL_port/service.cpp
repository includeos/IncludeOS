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

void Service::start()
{
  extern int init_ssl(const std::string&);
  init_ssl("www.google.com");

  // Get the first IP stack
  // It should have configuration from config.json
  auto& inet = net::Super_stack::get<net::IP4>(0);

  // Print some useful netstats every 30 secs
  using namespace std::chrono;
  Timers::periodic(5s, 30s,
  [&inet] (uint32_t) {
    printf("<Service> TCP STATUS:\n%s\n", inet.tcp().status().c_str());
  });

  // Set up a TCP server on port 80
  auto& server = inet.tcp().listen(80);

  // Add a TCP connection handler - here a hardcoded HTTP-service
  server.on_connect(
  [] (net::tcp::Connection_ptr conn)
  {
    printf("<Service> @on_connect: Connection %s successfully established.\n",
            conn->remote().to_string().c_str());
    // read async with a buffer size of 1024 bytes
    // define what to do when data is read
    conn->on_read(1024,
    [conn] (auto buf)
    {
      printf("<Service> @on_read: %u bytes received.\n", buf->size());
    });
    conn->on_write([](size_t written) {
      printf("<Service> @on_write: %u bytes written.\n", written);
    });
  });

  printf("*** Basic demo service started ***\n");
}
