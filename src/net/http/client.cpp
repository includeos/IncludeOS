// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#include <net/http/client.hpp>

#include <net/openssl/tls_stream.hpp>

namespace http {

  Client::Client(TCP& tcp, SSL_CTX* ssl_ctx, Request_handler on_send)
    : http::Basic_client(tcp, std::move(on_send), true),
      ssl_context(ssl_ctx)
  {
  }

  Client_connection& Client::get_secure_connection(const Host host)
  {
    // return/create a set for the given host
    auto& cset = conns_[host];

    // iterate all the connection and return the first free one
    for(auto& conn : cset)
    {
      if(!conn->occupied())
        return *conn;
    }

    auto tcp_stream = std::make_unique<net::tcp::Stream>(tcp_.connect(host));
    auto tls_stream = std::make_unique<openssl::TLS_stream>(ssl_context, std::move(tcp_stream), true);

    cset.push_back(std::make_unique<Client_connection>(*this, std::move(tls_stream)));

    return *cset.back();
  }

}
