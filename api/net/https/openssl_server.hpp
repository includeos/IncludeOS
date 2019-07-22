
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
