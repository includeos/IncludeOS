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

#pragma once
#ifndef NET_HTTP_CLIENT_HPP
#define NET_HTTP_CLIENT_HPP

#include <net/http/basic_client.hpp>
#include <openssl/ossl_typ.h>

namespace http {

  class Client : public http::Basic_client {
  public:
    explicit Client(TCP& tcp, SSL_CTX* ssl_ctx, Request_handler on_send = nullptr);

  private:
    SSL_CTX* ssl_context;

    virtual Client_connection& get_secure_connection(const Host host) override;

  }; // < class Client

} // < namespace http

#endif // < NET_HTTP_CLIENT_HPP
