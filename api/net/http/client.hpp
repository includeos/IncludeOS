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
#ifndef HTTP_CLIENT_HPP
#define HTTP_CLIENT_HPP

#include "common.hpp"
#include "request.hpp"
#include "response.hpp"
#include <net/tcp/tcp.hpp>
#include "connection.hpp"
#include "error.hpp"
#include <vector>
#include <map>

namespace http {

  class Client {
  public:
    using TCP               = net::TCP;
    using Host              = net::tcp::Socket;

    using Connection_set     = std::vector<std::unique_ptr<Connection>>;
    using Connection_mapset  = std::map<Host, Connection_set>;

    const static size_t bufsiz = 2048;

  private:
    using ResolveCallback    = delegate<void(net::ip4::Addr)>;

  public:
    explicit Client(TCP& tcp);

    void send(Request_ptr, Host host, Response_handler);

    void get(URI url, Header_set hfields, Response_handler cb);

    void get(std::string url, Header_set hfields, Response_handler cb)
    { get(URI{url}, std::move(hfields), std::move(cb)); }

    void get(Host host, std::string path, Header_set hfields, Response_handler cb);

    void post(URI url, Header_set hfields, std::string data, Response_handler cb);

    void post(std::string url, Header_set hfields, std::string data, Response_handler cb)
    { post(URI{url}, std::move(hfields), std::move(data), std::move(cb)); }

    void post(Host host, std::string path, Header_set hfields, const std::string& data, Response_handler cb);

    Request_ptr create_request() const;

  private:
    TCP& tcp_;
    Connection_mapset conns_;

    bool keep_alive_ = false;

    void resolve(const URI&, ResolveCallback);

    void set_connection_header(Request& req) const
    {
      req.header().set_field(header::Connection,
      (keep_alive_) ? std::string{"keep-alive"} : std::string{"close"});
    }

    /** Set uri and Host from URL */
    void populate_from_url(Request& req, const URI& url);

    /** Add data and content length */
    void add_data(Request&, const std::string& data);

    Connection& get_connection(const Host host);

    void close(Connection&);

  }; // < class Client

} // < namespace http

#endif // < HTTP_CLIENT_HPP
