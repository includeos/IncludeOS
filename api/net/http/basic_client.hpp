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
#ifndef HTTP_BASIC_CLIENT_HPP
#define HTTP_BASIC_CLIENT_HPP

// http
#include "client_connection.hpp"

#include <net/tcp/tcp.hpp>
#include <net/inet>
#include <vector>
#include <map>

namespace http {

  struct Client_error : public std::runtime_error {
    using runtime_error::runtime_error;
  };

  using Response_handler = Client_connection::Response_handler;

  class Basic_client {
  public:
    using TCP                 = net::TCP;
    using Host                = net::Socket;

    using Response_handler    = Client_connection::Response_handler;
    struct Options;
    using Request_handler     = delegate<void(Request&, Options&, const Host)>;

    using Connection_set      = std::vector<std::unique_ptr<Client_connection>>;
    using Connection_mapset   = std::map<Host, Connection_set>;

    using timeout_duration    = Client_connection::timeout_duration;

    const static timeout_duration     DEFAULT_TIMEOUT; // client.cpp, 5s
    static int                        default_follow_redirect; // 0

    /* Client Options */
    // aggregate initialization would make this pretty (c++20):
    // https://en.cppreference.com/w/cpp/language/aggregate_initialization
    struct Options {
      timeout_duration  timeout{DEFAULT_TIMEOUT};
      int               follow_redirect{default_follow_redirect};

      Options() noexcept {}

    };

  private:
    using ResolveCallback = net::Inet::resolve_func;

  public:
    explicit Basic_client(TCP& tcp, Request_handler on_send = nullptr);

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
    void send(Request_ptr req, Host host, Response_handler cb,
              const bool secure = false, Options options = {});

    /**
     * @brief      Send a request to a specific URL with a response handler
     *
     * @param[in]  req   The request
     * @param[in]  url   The URL
     * @param[in]  cb    Callback to be invoked when a response is received (or error)
     */
    void send(Request_ptr req, URI url, Response_handler cb, Options options = {});

    /**
     * @brief      Create a request on the given URL
     *
     * @param[in]  method   The HTTP method
     * @param[in]  url      The url
     * @param[in]  hfields  A set of headers
     * @param[in]  cb       Response handler
     */
    void request(Method method, URI url, Header_set hfields,
                 Response_handler cb, Options options = {});

    /**
     * @brief      Same as above
     */
    void request(Method method, std::string url, Header_set hfields,
                 Response_handler cb, Options options = {})
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
    void request(Method method, Host host, std::string path, Header_set hfields,
                 Response_handler cb, const bool secure = false, Options options = {});

    /**
     * @brief      Create a request on the given URL with payload
     *
     * @param[in]  method   The HTTP method
     * @param[in]  url      The url
     * @param[in]  hfields  A set of headers
     * @param[in]  data     The data (payload)
     * @param[in]  cb       Response handler
     */
    void request(Method method, URI url, Header_set hfields, std::string data,
                 Response_handler cb, Options options = {});

    /**
     * @brief      Same as above
     */
    void request(Method method, std::string url, Header_set hfields, std::string data,
                 Response_handler cb, Options options = {})
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
    void request(Method method, Host host, std::string path, Header_set hfields,
                 const std::string& data, Response_handler cb,
                 const bool secure = false, Options options = {});

    /* GET */
    inline void get(URI url, Header_set hfields, Response_handler cb, Options options = {});
    inline void get(Host host, std::string path, Header_set hfields,
                    Response_handler cb, const bool secure = false, Options options = {});

    /* POST */
    inline void post(URI url, Header_set hfields, std::string data,
                     Response_handler cb, Options options = {});
    inline void post(Host host, std::string path, Header_set hfields,
                     const std::string& data, Response_handler cb,
                     const bool secure = false, Options options = {});

    /**
     * @brief      Set callback to be invoked right before the request gets sent.
     *
     * @details    Useful for modifying/inspecting auto-generated Requests.
     *
     * @param[in]  cb    Request_handler callback
     */
    void on_send(Request_handler cb)
    { on_send_ = std::move(cb); }

    /**
     * @brief      Returns the Origin for the Client as a string.
     *             Currently returns the IP address to the stack.
     *
     * @return     The origin as a string
     */
    std::string origin() const
    { return tcp_.stack().ip_addr().to_string(); }

    virtual ~Basic_client() = default;

  protected:
    TCP&              tcp_;
    Connection_mapset conns_;

    explicit Basic_client(TCP& tcp, Request_handler on_send, const bool https_supported);

    virtual Client_connection& get_secure_connection(const Host host);

  private:
    friend class Client_connection;
    Request_handler   on_send_;
    bool              keep_alive_ = false;
    const bool        supports_https;

    void resolve(const std::string& host, ResolveCallback);

    void set_connection_header(Request& req) const
    {
      req.header().set_field(header::Connection,
      (keep_alive_) ? std::string{"keep-alive"} : std::string{"close"});
    }

    /** Set uri and Host from URL */
    static void populate_from_url(Request& req, const URI& url);

    /** Add data and content length */
    void add_data(Request&, const std::string& data);

    Client_connection& get_connection(const Host host);

    void close(Client_connection&);

    void validate_secure(const bool secure) const
    {
      if(secure and not this->supports_https)
        throw Client_error{"Secured connections not supported with Basic_client (use the HTTPS Client)."};
    }

  }; // < class Basic_client


  /* Inline implementation */
  inline void Basic_client::get(URI url, Header_set hfields, Response_handler cb, Options options)
  { request(GET, std::move(url), std::move(hfields), std::move(cb), std::move(options)); }

  inline void Basic_client::get(Host host, std::string path, Header_set hfields,
                          Response_handler cb, const bool secure, Options options)
  { request(GET, std::move(host), std::move(path), std::move(hfields), std::move(cb), secure, std::move(options)); }

  /* POST */
  inline void Basic_client::post(URI url, Header_set hfields, std::string data,
                           Response_handler cb, Options options)
  { request(POST, std::move(url), std::move(hfields), std::move(data), std::move(cb), std::move(options)); }

  inline void Basic_client::post(Host host, std::string path, Header_set hfields,
                           const std::string& data, Response_handler cb,
                           const bool secure, Options options)
  { request(POST, std::move(host), std::move(path), std::move(hfields), std::move(data), std::move(cb), secure, std::move(options)); }


} // < namespace http

#endif // < HTTP_BASIC_CLIENT_HPP
