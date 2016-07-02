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

#ifndef SERVER_CONNECTION_HPP
#define SERVER_CONNECTION_HPP

#include "net/tcp.hpp"
#include "request.hpp"
#include "response.hpp"

namespace server {

class Server;

class Connection;
using Connection_ptr = std::shared_ptr<Connection>;


class Connection {
private:
  const static size_t BUFSIZE = 1460;
  using Connection_ptr = net::TCP::Connection_ptr;
  using buffer_t = net::TCP::buffer_t;
  using OnData = net::TCP::Connection::ReadCallback;
  using Disconnect = net::TCP::Connection::Disconnect;
  using OnDisconnect = net::TCP::Connection::DisconnectCallback;
  using OnError = net::TCP::Connection::ErrorCallback;
  using TCPException = net::TCP::TCPException;
  using OnPacketDropped = net::TCP::Connection::PacketDroppedCallback;
  using Packet_ptr = net::TCP::Packet_ptr;

  using OnConnection = std::function<void()>;

public:
  Connection(Server&, Connection_ptr, size_t idx);

  Request_ptr get_request()
  { return request_; }

  Response_ptr get_response()
  { return response_; }


  void close();

  inline std::string to_string() const
  { return "Connection:[" + conn_->remote().to_string() + "]"; }

  static void on_connection(OnConnection cb)
  { on_connection_ = cb; }

  ~Connection();

private:
  Server& server_;
  Connection_ptr conn_;
  Request_ptr request_;
  Response_ptr response_;
  size_t idx_;

  static OnConnection on_connection_;

  void on_data(buffer_t, size_t);

  void on_disconnect(Connection_ptr, Disconnect);

  void on_error(Connection_ptr, TCPException);

  void on_packet_dropped(Packet_ptr, std::string);

}; // < server::Connection

}; // < server

#endif
