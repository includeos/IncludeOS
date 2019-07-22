#pragma once

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
