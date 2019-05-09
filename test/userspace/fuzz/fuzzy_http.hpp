#pragma once
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
#ifndef FUZZY_HTTP_SERVER_HPP
#define FUZZY_HTTP_SERVER_HPP

#include <net/http/server.hpp>

namespace fuzzy {

class HTTP_server : public http::Server
{
public:
  template <typename... Server_args>
  inline HTTP_server(
      net::TCP&   tcp,
      Server_args&&... server_args)
    : Server{tcp, std::forward<Server>(server_args)...}
  {}
  virtual ~HTTP_server() {}

  inline void add(net::Stream_ptr);

private:
  void bind(const uint16_t) override {}
  void on_connect(TCP_conn) override {}
};

inline void HTTP_server::add(net::Stream_ptr stream)
{
  this->connect(std::move(stream));
}

} // < namespace http

#endif
