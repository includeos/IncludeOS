
#pragma once
#ifndef MENDER_CLIENT_HPP
#define MENDER_CLIENT_HPP

#include "auth_manager.hpp"

namespace mender {

  static const std::string api_prefx = "/api/devices/0.1/";

	class Client {
  public:
    Client(Auth_manager&&);
    void auth(Auth_request&);

  private:
    Auth_manager am_;

    std::string build_url(const std::string& server) const;

    std::string build_api_url(const std::string& server, const std::string& url) const;

  }; // < class Client

  Client::Client(Auth_manager&& man)
    : am_{std::forward<Auth_manager>(man)}
  {
  }

  void Client::auth(Auth_request& auth)
  {
    //http::Request req;
    //req.add_header("X-MEN-Signature", signature(auth));
  }

}

#endif
