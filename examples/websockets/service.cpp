// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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

#include <service>
#include <net/inet4>
#include <deque>
#include <websocket>

static std::deque<net::WebSocket_ptr> websockets;

template <typename... Args>
static net::WebSocket_ptr& new_client(Args&&... args)
{
  for (auto& client : websockets)
  if (client->is_alive() == false) {
    return client = 
        net::WebSocket_ptr(new net::WebSocket(std::forward<Args> (args)...));
  }
  
  websockets.emplace_back(new net::WebSocket(std::forward<Args> (args)...));
  return websockets.back();
}

// verify clients connecting to server
bool accept_client(net::tcp::Socket remote, std::string origin)
{
  (void) origin;
  return remote.address() == net::ip4::Addr(10,0,0,1);
}

void websocket_service(net::Inet<net::IP4>& inet, uint16_t port)
{
  // buffer used for testing
  static net::tcp::buffer_t BUFFER;
  static const int          BUFLEN = 1000;
  BUFFER = decltype(BUFFER)(new uint8_t[BUFLEN]);
  
  /// server ///
  using namespace http;
  static auto server = 
      std::make_unique<Server>(inet.tcp(), nullptr, std::chrono::seconds(0));

  server->on_request(
  [] (Request_ptr req, Response_writer_ptr writer)
  {
    // create client websocket with accept function
    auto& socket = new_client(std::move(req), std::move(writer), accept_client);
    
    // if we are still connected, attempt was verified and the handshake was accepted
    if (socket->is_alive())
    {
      socket->on_read =
      [] (const char* data, size_t len) {
        (void) data;
        (void) len;
        printf("WebSocket on_read: %.*s\n", len, data);
      };
      // send one text message (most clients only support text)
      socket->write("THIS IS A TEST CAN YOU HEAR THIS?");
      // and then a few binary ones
      for (int i = 0; i < 1000; i++)
          socket->write(BUFFER, BUFLEN, net::WebSocket::BINARY);
      // close, eventually
      //socket->close();
    }
  });
  server->listen(port);
  /// server ///

  /// client ///
  static http::Client client(inet.tcp());
  // if you have a websocket server open on port 8001, 
  // this service will try to connect to it
  net::WebSocket::connect(client, "ws://10.0.0.42", "ws://10.0.0.1:8001/", 
  [] (net::WebSocket_ptr socket)
  {
    if (!socket) {
      printf("Connection failed!\n");
      return;
    }
    printf("Connected!\n");
    websockets.push_back(std::move(socket));
    websockets.back()->write("HOLAS");
  });
  /// client ///
}

void Service::start()
{
  // add own serial out after service start
  OS::add_stdout_default_serial();

  auto& inet = net::Inet4::ifconfig<>();
  inet.network_config(
      {  10, 0,  0, 42 },  // IP
      { 255,255,255, 0 },  // Netmask
      {  10, 0,  0,  1 },  // Gateway
      {  10, 0,  0,  1 }); // DNS

  // open a basic websocket server on port ...
  websocket_service(inet, 8000);
}
