
#pragma once
#ifndef UPLINK_WS_UPLINK_HPP
#define UPLINK_WS_UPLINK_HPP

#include "transport.hpp"

#include <net/inet4>
#include <net/http/client.hpp>
#include <net/http/websocket.hpp>

namespace uplink {

class WS_uplink {
public:
  static const std::string UPLINK_CFG_FILE;

  struct Config
  {
    std::string server;
    std::string password;
  };

  WS_uplink(net::Inet<net::IP4>&); 

  void start(net::Inet<net::IP4>&);

  void auth();

  void dock();

  void handle_transport(Transport_ptr);

private:
  std::unique_ptr<http::Client> client_;
  http::WebSocket_ptr           ws_;
  std::string                   token_;

  Config config_;

  Transport_parser parser_;

  void inject_token(http::Request& req, http::Client::Options&, const http::Client::Host)
  {
    if (not token_.empty())
      req.header().add_field("Authorization", "Bearer " + token_);
  }

  void handle_auth_response(http::Error err, http::Response_ptr res, http::Connection&);

  void establish_ws(http::WebSocket_ptr ws);

  void parse_transport(const char* data, size_t len);

  void read_config();

  void parse_config(const std::string& cfg);

}; // < class WS_uplink

} // < namespace uplink

#endif