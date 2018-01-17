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
#ifndef NET_HTTP_OPENSSL_SERVER_HPP
#define NET_HTTP_OPENSSL_SERVER_HPP

#include <net/http/server.hpp>

namespace http {

/**
 * @brief      An OpenSSL-based HTTPS server.
 */
class OpenSSL_server : public http::Server
{
public:
  /**
   * @brief      Construct a HTTPS server with the necessary certificates and keys.
   *
   * @param      ca_key       The ca key
   * @param      ca_cert      The ca cert
   * @param      tcp          The tcp
   * @param[in]  server_args  A list of args for constructing the underlying HTTP server
   *
   * @tparam     Server_args  Construct arguments to HTTP Server
   */
  template <typename... Server_args>
  inline OpenSSL_server(
      const std::string& ca_cert,
      const std::string& ca_key,
      net::TCP&   tcp,
      Server_args&&... server_args);

  virtual ~OpenSSL_server();

private:
  void* m_ctx = nullptr;

  void openssl_initialize(const std::string&, const std::string&);
  void bind(const uint16_t port) override;
  void on_connect(TCP_conn conn) override;
};

template <typename... Args>
inline OpenSSL_server::OpenSSL_server(
    const std::string& ca_key,
    const std::string& ca_cert,
    net::TCP&  tcp,
    Args&&...  server_args)
  : Server{tcp, std::forward<Server>(server_args)...}
{
  openssl_initialize(ca_key, ca_cert);
}

} // < namespace http

#endif
