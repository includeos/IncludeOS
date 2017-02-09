// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#pragma once
#ifndef NET_WEBSOCKETS_WEBSOCKET_HPP
#define NET_WEBSOCKETS_WEBSOCKET_HPP

#include <net/http/server.hpp>
#include <net/http/client.hpp>
#include <cstdint>
#include <delegate>
#include <memory>

namespace net {

struct WebSocket
{
  typedef std::unique_ptr<WebSocket> WebSocket_ptr;

  // client connected
  typedef delegate<void(WebSocket_ptr)> connect_func;
  // server new client
  typedef delegate<bool(tcp::Socket, std::string)> accept_func;
  // data read (data, length)
  typedef delegate<void(const char*, size_t)> read_func;
  // closed (status code)
  typedef delegate<void(uint16_t)>    close_func;
  // error (reason)
  typedef delegate<void(std::string)> error_func;

  /// Server-side connection
  WebSocket(http::Request_ptr         request,
            http::Response_writer_ptr response,
            accept_func = nullptr);
  /// Client-side connection
  static void
  connect(http::Client& client, 
          std::string   origin,
          uri::URI      dest,
          connect_func  callback);

  enum mode_t {
    TEXT,
    BINARY
  };
  void write(const char* buffer, size_t len, mode_t = TEXT);
  void write(net::tcp::buffer_t, size_t len, mode_t = TEXT);

  void write(const std::string& text)
  {
    write(text.c_str(), text.size(), TEXT);
  }

  // close the websocket
  void close();

  // user callbacks
  close_func   on_close = nullptr;
  error_func   on_error = nullptr;
  read_func    on_read  = nullptr;

  bool is_alive() const noexcept {
    return this->conn != nullptr;
  }
  bool is_client() const noexcept {
    return this->clientside;
  }
  const auto& get_connection() const noexcept {
    return this->conn;
  }

  // string description for status codes
  static const char* status_code(uint16_t code);

  WebSocket(tcp::Connection_ptr, bool);
  WebSocket(WebSocket&&);
  ~WebSocket();

private:
  tcp::Connection_ptr conn;
  bool clientside;

  WebSocket(const WebSocket&) = delete;
  WebSocket& operator= (const WebSocket&) = delete;
  WebSocket& operator= (WebSocket&&) = delete;
  void read_data(net::tcp::buffer_t, size_t);
  bool write_opcode(uint8_t code, const char*, size_t);
  void failure(const std::string&);
  void tcp_closed();
  void reset();
};
using WebSocket_ptr = WebSocket::WebSocket_ptr;
}

#endif
