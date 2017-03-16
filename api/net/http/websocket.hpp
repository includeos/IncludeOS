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
#ifndef NET_HTTP_WEBSOCKET_HPP
#define NET_HTTP_WEBSOCKET_HPP

#include <net/http/server.hpp>
#include <net/http/client.hpp>

namespace http {

class WebSocket {
public:
  using WebSocket_ptr   = std::unique_ptr<WebSocket>;
  // When a handshake is established and the WebSocket is created
  using Connect_handler = delegate<void(WebSocket_ptr)>;
  // Whether to accept the client or not before handshake
  using Accept_handler  = delegate<bool(net::tcp::Socket, std::string)>;
  // data read (data, length)
  typedef delegate<void(const char*, size_t)> read_func;
  // closed (status code)
  typedef delegate<void(uint16_t)>    close_func;
  // error (reason)
  typedef delegate<void(std::string)> error_func;

  /**
   * @brief      Upgrade a HTTP Request to a WebSocket connection.
   *
   * @param      req     The HTTP request
   * @param      writer  The HTTP response writer
   *
   * @return     A WebSocket_ptr, or nullptr if upgrade fails.
   */
  static WebSocket_ptr upgrade(Request& req, Response_writer& writer);

  /**
   * @brief      Upgrade a HTTP Response to a WebSocket connection.
   *
   * @param[in]  err   The HTTP error
   * @param      res   The HTTP response
   * @param      conn  The HTTP connection
   * @param      key   The WS key sent in the HTTP request
   *
   * @return     A WebSocket_ptr, or nullptr if upgrade fails.
   */
  static WebSocket_ptr upgrade(Error err, Response& res,
                               Connection& conn, const std::string& key);

  /**
   * @brief      Generate a random WebSocket key
   *
   * @return     A 16 char WebSocket key
   */
  static std::string generate_key();

  /**
   * @brief      Use a HTTP Client to connect to a WebSocket destination
   *
   * @param      client    The HTTP client
   * @param[in]  dest      The destination
   * @param[in]  callback  The connect callback
   */
  static void connect(http::Client&   client,
                      uri::URI        dest,
                      Connect_handler callback);

  /**
   * @brief      Creates a request handler on heap.
   *
   * @param[in]  on_connect  On connect handler
   * @param[in]  on_accept   On accept (optional)
   *
   * @return     A Request handler for a http::Server
   */
  static Server::Request_handler
  create_request_handler(Connect_handler on_connect,
                         Accept_handler  on_accept = nullptr);

  /**
   * @brief      Creates a response handler on heap.
   *
   * @param[in]  on_connect  On connect handler
   * @param[in]  key         The WebSocket key sent in outgoing HTTP header
   *
   * @return     A Response handler for a http::Client
   */
  static Client::Response_handler
  create_response_handler(Connect_handler on_connect, std::string key);

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
    return this->stream != nullptr;
  }
  bool is_client() const noexcept {
    return this->clientside;
  }
  const auto& get_connection() const noexcept {
    return this->stream;
  }

  // string description for status codes
  static const char* status_code(uint16_t code);

  WebSocket(net::Stream_ptr, bool);
  WebSocket(WebSocket&&);
  ~WebSocket();

private:
  net::Stream_ptr stream;
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

} // http

#endif
