// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_WS_WEBSOCKET_HPP
#define NET_WS_WEBSOCKET_HPP

#include "header.hpp"

#include <net/http/server.hpp>
#include <net/http/basic_client.hpp>
#include <memory>
#include <stdexcept>
#include <vector>

namespace net {

struct WS_error : public std::runtime_error {
  using base = std::runtime_error;
  using base::base;
};

class WebSocket {
public:
  class Message {
  public:
    using Data     = std::vector<uint8_t>;
    using Data_it  = Data::iterator;
    using Data_cit = Data::const_iterator;

    auto extract_vector() noexcept {
      return std::move(data_);
    }
    auto extract_shared_vector() {
      return std::make_shared<std::vector<uint8_t>> (std::move(data_));
    }

    std::string to_string() const
    { return std::string(data(), size()); }

    size_t size() const noexcept
    { return data_.size(); }

    Data_it begin() noexcept
    { return data_.begin(); }

    Data_it end() noexcept
    { return data_.end(); }

    Data_cit cbegin() const noexcept
    { return data_.begin(); }

    Data_cit cend() const noexcept
    { return data_.end(); }

    const char* data() const noexcept
    { return (const char*) data_.data(); }

    char* data() noexcept
    { return (char*) data_.data(); }

    Message(const uint8_t* data, size_t len)
    {
      const auto* wsh = (ws_header*) data;
      // setup initial header
      const size_t hdr_bytes = std::min((size_t) wsh->header_length(), len);
      std::memcpy(header_.data(), data, hdr_bytes);
      this->header_length = hdr_bytes;
      // move forward in buffer
      data += hdr_bytes; len -= hdr_bytes;
      // if the header became complete, reserve data
      if (this->header_complete()) {
        data_.reserve(header().data_length());
      }
      // append any remaining data
      this->append(data, len);
    }

    size_t append(const uint8_t* data, size_t len);

    bool is_complete() const noexcept
    { return header_complete() && data_.size() == header().data_length(); }

    const ws_header& header() const noexcept
    { return *(ws_header*) header_.data(); }

    op_code opcode() const noexcept
    { return header().opcode(); }

    void unmask() noexcept
    {
      if (header().is_masked())
          writable_header().masking_algorithm(this->data());
    }

  private:
    Data data_;
    std::array<uint8_t, 15> header_;
    uint8_t header_length = 0;

    inline bool header_complete() const noexcept {
      return header_length >= 2 && header_length >= header().header_length();
    }

    ws_header& writable_header()
    { return *(ws_header*) header_.data(); }

  }; // < class Message

  using Message_ptr     = std::unique_ptr<Message>;

  using WebSocket_ptr   = std::unique_ptr<WebSocket>;
  // When a handshake is established and the WebSocket is created
  using Connect_handler = delegate<void(WebSocket_ptr)>;
  // Whether to accept the client or not before handshake
  using Accept_handler  = delegate<bool(net::Socket, std::string)>;
  // data read (data, length)
  typedef delegate<void(Message_ptr)> read_func;
  // closed (status code)
  typedef delegate<void(uint16_t)>    close_func;
  // error (reason)
  typedef delegate<void(std::string)> error_func;
  // ping (return value is whether to return pong or not, default yes)
  typedef delegate<bool(const char* data, size_t len)> ping_func;
  // pong recv
  typedef delegate<void(const char* data, size_t len)> pong_func;
  // if a ping resulted in a timeout
  typedef delegate<void(WebSocket& ws)> pong_timeout_func;

  /**
   * @brief      Upgrade a HTTP Request to a WebSocket connection.
   *
   * @param      req     The HTTP request
   * @param      writer  The HTTP response writer
   *
   * @return     A WebSocket_ptr, or nullptr if upgrade fails.
   */
  static WebSocket_ptr upgrade(http::Request& req, http::Response_writer& writer);

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
  static WebSocket_ptr upgrade(http::Error err, http::Response& res,
                               http::Connection& conn, const std::string& key);

  /**
   * @brief      Generate a random WebSocket key
   *
   * @return     A 16 char WebSocket key
   */
  static std::vector<char> generate_key();

  /**
   * @brief      Use a HTTP Client to connect to a WebSocket destination
   *
   * @param      client    The HTTP client
   * @param[in]  dest      The destination
   * @param[in]  callback  The connect callback
   */
  static void connect(http::Basic_client&   client,
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
  static http::Server::Request_handler
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
  static http::Basic_client::Response_handler
  create_response_handler(Connect_handler on_connect, std::string key);

  void write(const char* buffer, size_t len, op_code = op_code::TEXT);
  void write(Stream::buffer_t, op_code = op_code::TEXT);

  void write(const std::string& text)
  {
    write(text.c_str(), text.size(), op_code::TEXT);
  }

  void write(const std::shared_ptr<std::vector<unsigned char>> data)
  {
    write((char *)data->data(),data->size());
  }

  bool ping(const char* buffer, size_t len, Timer::duration_t timeout)
  {
    ping_timer.start(timeout);
    return write_opcode(op_code::PING, buffer, len);
  }
  bool ping(Timer::duration_t timeout)
  { return ping(nullptr, 0, timeout); }

  //void ping(Stream::buffer_t, Timer::duration_t timeout);

  // close the websocket
  void close(uint16_t reason = 1000);

  void reset_callbacks();

  // user callbacks
  close_func   on_close = nullptr;
  error_func   on_error = nullptr;
  read_func    on_read  = nullptr;
  ping_func    on_ping  = {this, &WebSocket::default_on_ping};
  pong_func    on_pong  = nullptr;
  pong_timeout_func on_pong_timeout = nullptr;

  bool is_alive() const noexcept {
    return this->stream != nullptr;
  }
  bool is_client() const noexcept {
    return this->clientside;
  }
  const auto& get_connection() const noexcept {
    return this->stream;
  }

  // op code to string
  const char* to_string(op_code code);

  // string description for status codes
  static const char* status_code(uint16_t code);

  std::string to_string() const {
    return stream->to_string();
  }

  int get_cpuid() const noexcept {
    return stream->get_cpuid();
  }

  // 0 == unlimited
  void set_max_message_size(uint32_t sz) noexcept {
    max_msg_size = sz;
  }

  size_t serialize_to(void* p) const /*override*/;
  /* Create connection from binary data */
  static std::pair<WebSocket_ptr, size_t> deserialize_from(const void*);

  WebSocket(net::Stream_ptr, bool);
  ~WebSocket() {
    assert(m_busy == false && "Cannot delete stream while in its call stack");
  }

private:
  net::Stream_ptr stream;
  Timer ping_timer{{this, &WebSocket::pong_timeout}};
  Message_ptr message;
  uint32_t max_msg_size;
  bool     clientside;
  bool     m_busy = false;
  uint16_t m_deferred_close = 0;

  WebSocket(const WebSocket&) = delete;
  WebSocket(WebSocket&&) = delete;
  WebSocket& operator= (const WebSocket&) = delete;
  WebSocket& operator= (WebSocket&&) = delete;
  void read_data(Stream::buffer_t);
  bool write_opcode(op_code code, const char*, size_t);
  void failure(const std::string&);
  void close_callback_once();
  size_t create_message(const uint8_t*, size_t len);
  void finalize_message();

  bool default_on_ping(const char*, size_t)
  { return true; }

  void pong_timeout()
  {
    if (on_pong_timeout)
      on_pong_timeout(*this);
    else
      this->close(1000);
  }
};
using WebSocket_ptr = WebSocket::WebSocket_ptr;

} // http

#endif
