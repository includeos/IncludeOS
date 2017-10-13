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
#include <hw/devices.hpp>
#include <drivers/usernet.hpp>
#include <net/inet4>
#include <timers>

using namespace std::chrono;

inline UserNet& create_nic(){
  // the IncludeOS packet communicator
  auto* usernet = new UserNet();
  // register driver for superstack
  auto driver = std::unique_ptr<hw::Nic> (usernet);
  hw::Devices::register_device<hw::Nic> (std::move(driver));

  return *usernet;
}


void Service::start()
{


  // Create some pure userspace Nic's
  auto& nic1 = create_nic();
  auto& nic2 = create_nic();

  // Connect them with a wire
  nic1.set_transmit_forward({&nic2, &UserNet::receive});
  nic2.set_transmit_forward({&nic1, &UserNet::receive});

  // Create IP stacks on top of the nic's and configure them
  auto& inet_server = net::Super_stack::get<net::IP4>(0);
  auto& inet_client = net::Super_stack::get<net::IP4>(1);
  inet_server.network_config({10,0,0,42}, {255,255,255,0}, {10,0,0,1});
  inet_client.network_config({10,0,0,43}, {255,255,255,0}, {10,0,0,1});

  // Print some useful netstats every 30 secs
  Timers::periodic(5s, 30s,
  [&inet_server] (uint32_t) {
    printf("<Service> TCP STATUS:\n%s\n", inet_server.tcp().status().c_str());
  });

  // Set up a TCP server on port 80
  auto& server = inet_server.tcp().listen(80);

  // Add a TCP connection handler
  server.on_connect(
  [] (net::tcp::Connection_ptr conn) {
    printf("<Service> @on_connect: Connection %s successfully established.\n",
            conn->remote().to_string().c_str());
    // read async with a buffer size of 1024 bytes
    // define what to do when data is read
    conn->on_read(1024,
                  [conn] (auto buf)
    {
      const std::string data((const char*) buf->data(), buf->size());
      printf("Server received data: \n%s\n", data.c_str());
    });
  });

  printf("*** Linux userspace library demo started ***\n");

  inet_client.tcp().connect({{"10.0.0.42"}, 80}, [](auto conn){
      if (not conn)
        std::abort();

      conn->write("Hello world\n");
    });

  fflush(stdout);
}
