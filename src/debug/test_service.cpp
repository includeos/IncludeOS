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
#include <net/dhcp/dh4client.hpp>
#include "ircd.hpp"

using namespace std::chrono;

// An IP-stack object
std::unique_ptr<net::Inet4<VirtioNet> > inet;

void Service::start()
{
  // boilerplate
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0);
  inet->network_config(
   {{ 10,0,0,42 }},      // IP
   {{ 255,255,255,0 }},  // Netmask
   {{ 10,0,0,1 }},       // Gateway
   {{ 8,8,8,8 }} );      // DNS
  
  /*
  auto& tcp = inet->tcp();
  auto& server = tcp.bind(6667); // IRCd default port
  server.onConnect(
  [] (auto csock)
  {
    printf("*** Received connection from %s\n",
    csock->remote().to_string().c_str());
    
    /// create client ///
    size_t index = clients.size();
    clients.emplace_back(index, csock);
    
    auto& client = clients[index];
    
    // set up callbacks
    csock->onReceive(
    [&client] (auto conn, bool)
    {
    char buffer[1024];
    size_t bytes = conn->read(buffer, sizeof(buffer));
      
    client.read(buffer, bytes);
      
    });
    
    .onDisconnect(
    [&client] (auto conn, std::string)
    {
    // remove client from various lists
    client.remove();
    /// inform others about disconnect
    //client.bcast(TK_QUIT, "Disconnected");
    });
  });*/
  
  using namespace net;
  const UDP::port_t port = 4242;
  auto& sock = inet->udp().bind(port);
  
  sock.on_read(
  [&sock] (UDP::addr_t addr, UDP::port_t port,
           const char* data, size_t len)
  {
    std::string strdata(data, len);
    CHECK(1, "Getting UDP data from %s:%d -> %s",
              addr.str().c_str(), port, strdata.c_str());
    // send the same thing right back!
    sock.sendto(addr, port, data, len,
    [&sock, addr, port]
    {
      // print this message once
      printf("*** Starting spam (you should see this once)\n");
      
      typedef std::function<void()> rnd_gen_t;
      auto next = std::make_shared<rnd_gen_t> ();
      
      *next = 
      [next, &sock, addr, port] ()
      {
        // spam this message at max speed
        std::string text("Spamorino Cappucino\n");
        
        sock.sendto(addr, port, text.data(), text.size(),
        [next] { (*next)(); });
      };
      
      // start spamming
      (*next)();
    });
  });
  
  printf("*** TEST SERVICE STARTED *** \n");
}
