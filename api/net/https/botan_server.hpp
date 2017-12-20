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
#ifndef NET_BOTAN_HTTP_SERVER_HPP
#define NET_BOTAN_HTTP_SERVER_HPP

#include <net/http/server.hpp>
#include <fs/dirent.hpp>
#include <net/botan/tls_server.hpp>

namespace http {

/**
 * @brief      A secure HTTPS server.
 */
class Botan_server : public http::Server
{
public:
  /**
   * @brief      Construct a HTTPS server with the necessary certificates and keys.
   *
   * @param[in]  name         The name
   * @param      ca_key       The ca key
   * @param      ca_cert      The ca cert
   * @param      server_key   The server key
   * @param      tcp          The tcp
   * @param[in]  server_args  A list of args for constructing the underlying HTTP server
   *
   * @tparam     Server_args  Construct arguments to HTTP Server
   */
  template <typename... Server_args>
  inline Botan_server(
      const std::string& name,
      fs::Dirent& ca_key,
      fs::Dirent& ca_cert,
      fs::Dirent& server_key,
      net::TCP&   tcp,
      Server_args&&... server_args);

  /**
   * @brief      Construct a HTTPS server with a credential manager and rng.
   *
   * @param      in_credman   In credman
   * @param      in_rng       In random number generator
   * @param      tcp          The tcp
   * @param[in]  server_args  A list of args for constructing the underlying HTTP server
   *
   * @tparam     Server_args  Server_args  Construct arguments to HTTP Server
   */
  template <typename... Server_args>
  inline Botan_server(
      Botan::Credentials_Manager*   in_credman,
      Botan::RandomNumberGenerator& in_rng,
      net::TCP& tcp,
      Server_args&&... server_args);

  /**
   * @brief      Loads credentials.
   *
   * @param[in]  name        The name
   * @param      ca_key      The ca key
   * @param      ca_cert     The ca cert
   * @param      server_key  The server key
   */
  void load_credentials(
      const std::string& name,
      fs::Dirent& ca_key,
      fs::Dirent& ca_cert,
      fs::Dirent& server_key);

private:
  Botan::RandomNumberGenerator& rng;
  std::unique_ptr<Botan::Credentials_Manager> credman;

  /**
   * @brief      Binds TCP to pass all new connections to this on_connect.
   *
   * @param[in]  port  The port
   */
  void bind(const uint16_t port) override;

  /**
   * @brief      Try to upgrade a newly established TCP connection to a TLS connection.
   *
   * @param[in]  conn  The TCP connection
   */
  void on_connect(TCP_conn conn) override;

  /**
   * @brief      Gets the random number generator.
   *
   * @return     The random number generator.
   */
  static Botan::RandomNumberGenerator& get_rng();

}; // < class Botan_server

template <typename... Server_args>
inline Botan_server::Botan_server(
    const std::string& name,
    fs::Dirent& ca_key,
    fs::Dirent& ca_cert,
    fs::Dirent& server_key,
    net::TCP&   tcp,
    Server_args&&... server_args)
  : Server{tcp, std::forward<Server>(server_args)...},
    rng(get_rng())
{
  load_credentials(name, ca_key, ca_cert, server_key);
}

template <typename... Server_args>
inline Botan_server::Botan_server(
    Botan::Credentials_Manager*   in_credman,
    Botan::RandomNumberGenerator& in_rng,
    net::TCP& tcp,
    Server_args&&... server_args)
  : Server{tcp, std::forward(server_args)...},
    rng(in_rng), credman(in_credman)
{
  assert(credman != nullptr);
}

} // < namespace http

#endif
