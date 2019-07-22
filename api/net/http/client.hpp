
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
