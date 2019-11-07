
#pragma once
#ifndef NET_HTTP_S2N_SERVER_HPP
#define NET_HTTP_S2N_SERVER_HPP

#include <net/http/server.hpp>

namespace http {

/**
 * @brief      An S2N-based HTTPS server.
 */
class S2N_server : public http::Server
{
public:
  template <typename... Server_args>
  inline S2N_server(
      const std::string& ca_cert,
      const std::string& ca_key,
      net::TCP&   tcp,
      Server_args&&... server_args);

  virtual ~S2N_server();

private:
  void* m_config = nullptr;

  void initialize(const std::string&, const std::string&);
  void bind(const uint16_t port) override;
  void on_connect(TCP_conn conn) override;
};

template <typename... Args>
inline S2N_server::S2N_server(
    const std::string& ca_key,
    const std::string& ca_cert,
    net::TCP&  tcp,
    Args&&...  server_args)
  : Server{tcp, std::forward<Server>(server_args)...}
{
  this->initialize(ca_key, ca_cert);
}

} // < namespace http

#endif
