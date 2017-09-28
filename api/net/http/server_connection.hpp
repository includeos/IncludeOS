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
#ifndef HTTP_SERVER_CONNECTION_HPP
#define HTTP_SERVER_CONNECTION_HPP

// http
#include "connection.hpp"

#include <rtc>

namespace http {

  class Server;

  class Server_connection : public Connection {
  public:
    static constexpr size_t DEFAULT_BUFSIZE = 1460;

  public:
    explicit Server_connection(Server&, Stream_ptr, size_t idx, const size_t bufsize = DEFAULT_BUFSIZE);

    void send(Response_ptr res);

    size_t idx() const noexcept
    { return idx_; }

    auto idle_since() const noexcept
    { return idle_since_; }

  private:
    Server&           server_;
    Request_ptr       req_;
    size_t            idx_;
    RTC::timestamp_t  idle_since_;

    void recv_request(buffer_t);

    void end_request(status_t code = http::OK);

    void close() override;

    void update_idle()
    { idle_since_ = RTC::now(); }

  }; // < class Server_connection



} // < namespace http

#endif // < HTTP_SERVER_CONNECTION_HPP
