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

#include "request.hpp"
#include "response.hpp"
#include <net/tcp/tcp.hpp>

namespace http {

  class Error {
  public:
    enum Code {
      NONE,
      RESOLVE_HOST,
      NO_REPLY
    };

    Error(Code code = NONE)
      : code_{code}
    {}

    operator bool() const
    { return code_ != NONE; }

    Error code() const
    { return code_; }

    std::string to_string() const
    {
      switch(code_)
      {
        case NONE:
          return "No error";
        case RESOLVE_HOST:
          return "Unable to resolve host";
        case NO_REPLY:
          return "No reply";
        default:
          return "General error";
      } // < switch code_
    }

  private:
    Code code_;

  };

  class Client {
  public:
    using URI               = uri::URI;
    using TCP               = net::TCP;
    using Host              = net::tcp::Socket;
    using Request_ptr       = std::unique_ptr<Request>;
    using Response_ptr      = std::unique_ptr<Response>;
    using ResponseCallback  = delegate<void(Error, Response_ptr)>;
    const static size_t bufsiz = 4096;

  private:
    using Connection_ptr    = net::tcp::Connection_ptr;
    using ResolveCallback   = delegate<void(net::ip4::Addr)>;


  public:
    explicit Client(TCP& tcp);

    void send(Request_ptr, Host host, ResponseCallback);

    void get(URI url, Header_set hfields, ResponseCallback cb);

    void post(URI url, Header_set hfields, std::string data, ResponseCallback cb);

    Request_ptr create_request() const;

  private:
    TCP& tcp_;
    Connection_ptr conn_;
    bool keep_alive_ = false;

    void resolve(const URI&, ResolveCallback);

    void set_connection_header(Request& req) const
    {
      req.header().set_field(header::Connection,
      (keep_alive_) ? std::string{"keep-alive"} : std::string{"close"});
    }

    void populate_from_url(Request& req, const URI& url);

  }; // < class Client

} // < namespace http

#endif // < HTTP_CLIENT_HPP
