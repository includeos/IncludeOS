// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#include <net/inet>
#include <service>

#include <net/ws/websocket.hpp>
void handle_ws(net::WebSocket_ptr ws)
{
  static std::map<int, net::WebSocket_ptr> websockets;
  static int idx = 0;

  // nullptr means the WS attempt failed
  if(not ws) {
    printf("WS failed\n");
    return;
  }
  printf("WS Connected: %s\n", ws->to_string().c_str());

  // Write a welcome message
  ws->write("Welcome");
  ws->write(ws->to_string());
  // Setup echo reply
  ws->on_read = [ws = ws.get()] (auto msg) {
    printf("WS Recv: %s\n", msg->as_text().c_str());
    // Extracting the data from the message is performant
    ws->write(msg->extract_shared_vector());
  };
  ws->on_close = [ws = ws.get()](auto code) {
  // Notify on close
    printf("WS Closing (%u) %s\n", code, ws->to_string().c_str());
  };

  websockets[idx++] = std::move(ws);
}

#include <net/http/server.hpp>
#include <memdisk>
std::unique_ptr<http::Server> server;

void Service::start()
{
  // Retreive the stack (configured from outside)
  auto& inet = net::Inet::stack<0>();
  Expects(inet.is_configured());

  // Init the memdisk
  auto& disk = fs::memdisk();
  disk.init_fs([] (auto err, auto&) {
    Expects(not err);
  });
  // Retreive the HTML page from the disk
  auto file = disk.fs().read_file("/index.html");
  Expects(file.is_valid());
  net::tcp::buffer_t html(
      new std::vector<uint8_t> (file.data(), file.data() + file.size()));

  // Create a HTTP Server and setup request handling
  server = std::make_unique<http::Server>(inet.tcp());
  server->on_request([html] (auto req, auto rw)
  {
    // We only support get
    if(req->method() != http::GET) {
      rw->write_header(http::Not_Found);
      return;
    }
    // Serve HTML on /
    if(req->uri() == "/") {
      rw->write(html);
    }
    // WebSockets go here
    else if(req->uri() == "/ws") {
      auto ws = net::WebSocket::upgrade(*req, *rw);
      handle_ws(std::move(ws));
    }
    else {
      rw->write_header(http::Not_Found);
    }
  });

  // Start listening on port 80
  server->listen(80);
}
