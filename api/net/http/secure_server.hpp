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
#ifndef NET_HTTP_SECURE_SERVER_HPP
#define NET_HTTP_SECURE_SERVER_HPP

#include <net/http/server.hpp>
#include <fs/dirent.hpp>
#include <net/tls/server.hpp>

namespace http {

class Secure_server : public http::Server
{
public:
  Secure_server(
      fs::Dirent& ca_key,
      fs::Dirent& ca_cert,
      fs::Dirent& server_key,
      TCP& tcp,
      Request_handler cb
    );

  Secure_server(
      Botan::Credentials_Manager*   in_credman,
      Botan::RandomNumberGenerator& in_rng,
      TCP& tcp,
      Request_handler cb)
    : http::Server(tcp, cb), rng(in_rng), credman(in_credman)
  {
    on_connect = {this, &Secure_server::secure_connect};
  }

  void secure_connect(TCP_conn conn)
  {
    auto* ptr = new net::tls::Server(conn, rng, *credman);

    ptr->on_connect(
    [this, ptr] (net::Stream&)
    {
      // create and pass TLS socket
      Server::connect(std::unique_ptr<net::tls::Server>(ptr));
    });
    ptr->on_close([ptr] {
      printf("Secure_HTTP::on_close on %s\n", ptr->to_string().c_str());
      delete ptr;
    });
  }

private:
  Botan::RandomNumberGenerator& rng;
  std::unique_ptr<Botan::Credentials_Manager> credman;
};

} // http

#endif
