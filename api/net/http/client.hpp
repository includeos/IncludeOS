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

    using timeout_duration  = Connection::timeout_duration;

    const static timeout_duration     DEFAULT_TIMEOUT; // client.cpp, 5s
    constexpr static size_t           DEFAULT_BUFSIZE = 2048;

    /* Client Options */
    // if someone has a better solution, please fix
    struct Options {
      timeout_duration  timeout{DEFAULT_TIMEOUT};
      size_t            bufsize{DEFAULT_BUFSIZE};

      Options(timeout_duration dur, size_t bufsz)
        : timeout{dur},
          bufsize{bufsz}
      {}

      Options() : Options(DEFAULT_TIMEOUT, DEFAULT_BUFSIZE) {}

      Options(timeout_duration dur) : Options(dur, DEFAULT_BUFSIZE) {}

      Options(size_t bufsz) : Options(DEFAULT_TIMEOUT, bufsz) {}

    };

  private:
    using ResolveCallback    = delegate<void(net::ip4::Addr)>;

  public:
    explicit Client(TCP& tcp);

    /**
     * @brief      Creates a request with some predefined attributes
     *
     * @param[in]  method  The HTTP method
     *
     * @return     A Request_ptr
     */
    Request_ptr create_request(Method method = GET) const;

    /**
     * @brief      Send a request to a specific host with a response handler
     *
     * @param[in]  req   The request
     * @param[in]  host  The host
     * @param[in]  cb    Callback to be invoked when a response is received (or error)
     */
    void send(Request_ptr req, Host host, Response_handler cb, Options options = {});

    /**
     * @brief      Create a request on the given URL
     *
     * @param[in]  method   The HTTP method
     * @param[in]  url      The url
     * @param[in]  hfields  A set of headers
     * @param[in]  cb       Response handler
     */
    void request(Method method, URI url, Header_set hfields, Response_handler cb, Options options = {});

    /**
     * @brief      Same as above
     */
    void request(Method method, std::string url, Header_set hfields, Response_handler cb, Options options = {})
    { request(method, URI{url}, std::move(hfields), std::move(cb), std::move(options)); }

    /**
     * @brief      Create a request to the given host on the given path
     *
     * @param[in]  method   The HTTP method
     * @param[in]  host     The host
     * @param[in]  path     The path
     * @param[in]  hfields  A set of headers
     * @param[in]  cb       Response handler
     */
    void request(Method method, Host host, std::string path, Header_set hfields, Response_handler cb, Options options = {});

    /**
     * @brief      Create a request on the given URL with payload
     *
     * @param[in]  method   The HTTP method
     * @param[in]  url      The url
     * @param[in]  hfields  A set of headers
     * @param[in]  data     The data (payload)
     * @param[in]  cb       Response handler
     */
    void request(Method method, URI url, Header_set hfields, std::string data, Response_handler cb, Options options = {});

    /**
     * @brief      Same as above
     */
    void request(Method method, std::string url, Header_set hfields, std::string data, Response_handler cb, Options options = {})
    { request(method, URI{url}, std::move(hfields), std::move(data), std::move(cb), std::move(options)); }

    /**
     * @brief      Create a request to the given host on the given path with payload
     *
     * @param[in]  method   The HTTP method
     * @param[in]  host     The host
     * @param[in]  path     The path
     * @param[in]  hfields  A set of headers
     * @param[in]  data     The data (payload)
     * @param[in]  cb       Response handler
     */
    void request(Method method, Host host, std::string path, Header_set hfields, const std::string& data, Response_handler cb, Options options = {});

    /* GET */
    inline void get(URI url, Header_set hfields, Response_handler cb, Options options = {});
    inline void get(std::string url, Header_set hfields, Response_handler cb, Options options = {});
    inline void get(Host host, std::string path, Header_set hfields, Response_handler cb, Options options = {});

    /* POST */
    inline void post(URI url, Header_set hfields, std::string data, Response_handler cb, Options options = {});
    inline void post(std::string url, Header_set hfields, std::string data, Response_handler cb, Options options = {});
    inline void post(Host host, std::string path, Header_set hfields, const std::string& data, Response_handler cb, Options options = {});

  private:
    TCP& tcp_;
    Connection_mapset conns_;

    bool keep_alive_ = false;

    void resolve(const std::string& host, ResolveCallback);

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


  /* Inline implementation */
  inline void Client::get(URI url, Header_set hfields, Response_handler cb, Options options)
  { request(GET, std::move(url), std::move(hfields), std::move(cb), std::move(options)); }

  inline void Client::get(std::string url, Header_set hfields, Response_handler cb, Options options)
  { get(URI{std::move(url)}, std::move(hfields), std::move(cb), std::move(options)); }

  inline void Client::get(Host host, std::string path, Header_set hfields, Response_handler cb, Options options)
  { request(GET, std::move(host), std::move(path), std::move(hfields), std::move(cb), std::move(options)); }

  /* POST */
  inline void Client::post(URI url, Header_set hfields, std::string data, Response_handler cb, Options options)
  { request(POST, std::move(url), std::move(hfields), std::move(data), std::move(cb), std::move(options)); }

  inline void Client::post(std::string url, Header_set hfields, std::string data, Response_handler cb, Options options)
  { post(URI{std::move(url)}, std::move(hfields), std::move(data), std::move(cb), std::move(options)); }

  inline void Client::post(Host host, std::string path, Header_set hfields, const std::string& data, Response_handler cb, Options options)
  { request(POST, std::move(host), std::move(path), std::move(hfields), std::move(data), std::move(cb), std::move(options)); }


} // < namespace http

#endif // < HTTP_CLIENT_HPP
