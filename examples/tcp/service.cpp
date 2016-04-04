// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

/*
  An example to show incoming and outgoing TCP Connections.
  In this example, IncludeOS is listening on port 80.
        
  Data received on port 80 will be redirected to a
  outgoing connection to a (in this case) python server (server.py)
        
  Data received from the python server connection 
  will be redirected back to the client.

  To try it out, use netcat to connect to this IncludeOS instance.
*/

#include <os>
#include <net/inet4>

using Connection_ptr = std::shared_ptr<net::TCP::Connection>;
using Disconnect = net::TCP::Connection::Disconnect;

// An IP-stack object
std::unique_ptr<net::Inet4<VirtioNet> > inet;

// Address to our python server: 10.0.2.2:1337
// @note: This may be needed to be modified depending on network and server settings.
net::TCP::Socket python_server{ {{10,0,2,2}} , 1337};

// Called when data is received on client (incoming connection)
void handle_client_on_receive(Connection_ptr client, Connection_ptr python) {
  // Read the request from our client
  std::string request = client->read(1024);
  printf("Received [Client]: %s\n", request.c_str());
  // Write the request to our python server
  python->write(request);
}

// Called when data is received on python (outgoing connection)
void handle_python_on_receive(Connection_ptr python, Connection_ptr client) {
  // Read the response from our python server
  std::string response = python->read(1024);
  // Write response to our client
  client->write(response);
}

void Service::start() {
  // Assign a driver (VirtioNet) to a network interface (eth0)
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  
  // Bring up a network stack, attached to the nic
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);
  
  // Static IP configuration, until we (possibly) get DHCP
  inet->network_config( {{ 10,0,0,42 }},      // IP
                        {{ 255,255,255,0 }},  // Netmask
                        {{ 10,0,0,1 }},       // Gateway
                        {{ 8,8,8,8 }} );      // DNS

  // Set up a TCP server on port 80
  auto& server = inet->tcp().bind(80);
  inet->dhclient()->on_config([&server](auto&) {
      printf("Server IP updated: %s\n", server.local().to_string().c_str());
    });
  printf("Server listening: %s \n", server.local().to_string().c_str());
  // When someone connects to our server
  server.onConnect([](Connection_ptr client) {
      printf("Connected [Client]: %s\n", client->to_string().c_str());
      // Make an outgoing connection to our python server
      auto outgoing = inet->tcp().connect(python_server);
      // When outgoing connection to python sever is established
      outgoing->onConnect([client](Connection_ptr python) {
          printf("Connected [Python]: %s\n", python->to_string().c_str());

          // Setup handlers for when data is received on client and python connection
          // When client has data to be read
          client->onReceive([python](Connection_ptr client, bool) {
              handle_client_on_receive(client, python);
            });

          // When python server has data to be read
          python->onReceive([client](Connection_ptr python, bool) {
              handle_python_on_receive(python, client);
            });

          // When client is disconnecting
          client->onDisconnect([python](Connection_ptr, Disconnect reason) {
              printf("Disconnected [Client]: %s\n", reason.to_string().c_str());
              python->close();
            });

          // When python is disconnecting
          python->onDisconnect([client](Connection_ptr, Disconnect reason) {
              printf("Disconnected [Python]: %s\n", reason.to_string().c_str());
              client->close();
            });
        }); // << onConnect (outgoing (python))
    }); // << onConnect (client)

}
