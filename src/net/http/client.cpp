
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
