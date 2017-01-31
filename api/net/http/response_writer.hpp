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
#ifndef HTTP_RESPONSE_WRITER_HPP
#define HTTP_RESPONSE_WRITER_HPP

// http
#include "response.hpp"
#include <net/tcp/connection.hpp>

namespace http {

  class Response_writer {
  public:
    using TCP_conn  = net::tcp::Connection_ptr;
    using buffer_t  = net::tcp::buffer_t;

  public:
    Response_writer(Response_ptr res, TCP_conn);

    auto& header()
    { return response_->header(); }

    const auto& header() const
    { return response_->header(); }

    void send_header(status_t code = http::OK);

    void send();

    void send_body(std::string data);

    void send(buffer_t buf, size_t len);

    auto& res()
    { return *response_; }

    auto& conn()
    { return connection_; }

  private:
    Response_ptr  response_;
    TCP_conn      connection_;
    bool          header_sent_{false};

  }; // < class Response_writer

} // < namespace http

#endif // < HTTP_RESPONSE_WRITER_HPP
