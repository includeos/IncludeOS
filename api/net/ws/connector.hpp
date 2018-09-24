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

#pragma once
#ifndef NET_HTTP_WS_CONNECTOR_HPP
#define NET_HTTP_WS_CONNECTOR_HPP

#include <net/ws/websocket.hpp>

namespace net {

/**
 * @brief      A WebSocket connector.
 *             Plugin/translation from HTTP to WebSocket
 */
class WS_connector {
public:
  using ConnectCallback   = delegate<void(WebSocket_ptr)>;

protected:
  ConnectCallback on_connect_;

  WS_connector(ConnectCallback on_connect)
    : on_connect_(std::move(on_connect)) {}

}; // WS_connector

/**
 * @brief      WebSocket server connector used with HTTP servers.
 */
class WS_server_connector : public WS_connector {
public:
  using AcceptCallback    = delegate<bool(net::Socket peer, const std::string& origin)>;
  using Request_handler   = delegate<void(http::Request_ptr, http::Response_writer_ptr)>;

  /**
   * @brief      Construct a server connector.
   *
   * @param[in]  on_connect  On connect callback
   * @param[in]  on_accept   On accept (optional)
   */
  WS_server_connector(ConnectCallback on_connect, AcceptCallback on_accept = nullptr)
    : WS_connector(std::move(on_connect)),
      on_accept_(std::move(on_accept))
  {
  }

  /**
   * @brief      Creates a HTTP Request handler to be used with servers.
   *
   * @return     A HTTP Request handler
   */
  Request_handler create_request_handler()
  { return {this, &WS_server_connector::on_request}; }

  operator Request_handler()
  { return create_request_handler(); }

  /**
   * @brief      Handles a HTTP Request by trying to upgrade it to a WebSocket.
   *             Calls "on_connect" with the WS, nullptr if failed.
   *
   * @param[in]  req     The HTTP request
   * @param[in]  writer  The HTTP Response writer
   */
  void on_request(http::Request_ptr req, http::Response_writer_ptr writer)
  {
    if (on_accept_)
    {
      const bool accepted = on_accept_(writer->connection().peer(),
                                     std::string(req->header().value("Origin")));
      if (not accepted)
      {
        writer->write_header(http::Unauthorized);
        on_connect_(nullptr);
        return;
      }
    }
    auto ws = WebSocket::upgrade(*req, *writer);
    if (ws == nullptr) return;

    assert(ws->get_cpuid() == SMP::cpu_id());
    on_connect_(std::move(ws));
  }

private:
  AcceptCallback  on_accept_;

}; // < WS_server_connector

/**
 * @brief      WebSocket server connector used with HTTP clients.
 *             This one is a bit messier due to the websocket key...
 */
class WS_client_connector : public WS_connector {
public:
  using Response_handler  = delegate<void(http::Error, http::Response_ptr, http::Connection&)>;

  /**
   * @brief      Construct a client connector.
   *             This one acts more like a factory tho.
   *
   * @param[in]  on_connect  On connect callback
   */
  WS_client_connector(ConnectCallback on_connect)
    : WS_connector(std::move(on_connect)),
      key_{}
  {
  }

  /**
   * @brief      Creates a HTTP Response handler by allocating a client connector
   *             with restricted lifetime to the response handler itself.
   *
   * @param[in]  cb    A connect callback
   * @param[in]  key   The WS key
   *
   * @return     Returns a Response handler with a captured client connector.
   */
  static Response_handler create_response_handler(ConnectCallback cb, std::string key)
  {
    // @todo Try replace with unique_ptr
    // create a new instance of a client connector
    //auto ptr = std::unique_ptr<WS_client_connector>{
    //  new WS_client_connector(std::move(cb), std::move(key))
    //};
    auto ptr = std::make_shared<WS_client_connector>(std::move(cb), std::move(key));

    return [ ptr{std::move(ptr)} ]
           (auto err, auto res, auto& conn)
           {
            ptr->on_response(err, std::move(res), conn);
           };
  }

  /**
   * @brief      Creates a response handler with the current connect callback.
   *             See the static variant for more info.
   *
   * @param[in]  key   The WS key
   *
   * @return     Returns a Response handler with a captured client connector.
   */
  Response_handler create_response_handler(std::string key)
  {
    return create_response_handler(on_connect_, std::move(key));
  }

  /**
   * @brief      Handles a HTTP Response by trying to upgrade it to a WebSocket.
   *             Calls "on_connect" with the WS, nullptr if failed.
   *
   * @param[in]  err   The HTTP error
   * @param[in]  res   The HTTP response
   * @param      conn  The HTTP connection
   */
  void on_response(http::Error err, http::Response_ptr res, http::Connection& conn)
  {
    auto ws = WebSocket::upgrade(err, *res, conn, key_);

    if(ws == nullptr) {
    } // not ok

    on_connect_(std::move(ws));
  }

  // Either use the ones above, or these below.
  /**
   * @brief      Constructor to be used when constructing unique instances
   *             of the client connector.
   *
   * @param[in]  on_connect  On connect callback
   * @param[in]  key         The WS key
   */
  WS_client_connector(ConnectCallback on_connect, std::string key)
    : WS_connector(std::move(on_connect)),
      key_(std::move(key))
  {
  }

  /**
   * @brief      Creates a response handler based on the key and connect callback
   *             set in the unique instance of the client connector.
   *
   * @return     Returns a response handler pointing to this object.
   */
  Response_handler create_response_handler()
  {
    return {this, &WS_client_connector::on_response};
  }

private:
  std::string key_;

}; // < WS_client_connector

} // < net

#endif // NET_HTTP_WS_CONNECTOR_HPP
