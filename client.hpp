
#pragma once
#ifndef MENDER_CLIENT_HPP
#define MENDER_CLIENT_HPP

#include "auth_manager.hpp"
#include <net/tcp/tcp.hpp>
#include <string>
#include <botan/base64.h>
#include <net/http/client.hpp>

namespace mender {

  static const std::string api_prefx = "/api/devices/0.1/";

	class Client {
  public:
    using AuthCallback  = delegate<void(bool)>;
  public:
    Client(Auth_manager&&, net::TCP&, const std::string& server, const uint16_t port = 0);
    Client(Auth_manager&&, net::TCP&, net::tcp::Socket);
    void make_auth_request(AuthCallback);

  private:
    const std::string server_;
    Auth_manager am_;
    net::tcp::Socket cached_;
    std::unique_ptr<http::Client> httpclient_;

    std::string build_url(const std::string& server) const;

    std::string build_api_url(const std::string& server, const std::string& url) const;

  }; // < class Client

  Client::Client(Auth_manager&& man, net::TCP& tcp, const std::string& server, const uint16_t port)
    : server_(server),
      am_{std::forward<Auth_manager>(man)},
      cached_{0, port},
      httpclient_{std::make_unique<http::Client>(tcp)}
  {

  }

  Client::Client(Auth_manager&& man, net::TCP& tcp, net::tcp::Socket socket)
    : server_{socket.address().to_string()},
      am_{std::forward<Auth_manager>(man)},
      cached_(std::move(socket)),
      httpclient_{std::make_unique<http::Client>(tcp)}
  {

  }

  void Client::make_auth_request(AuthCallback cb)
  {
    auto auth = am_.make_auth_request();

    using namespace std::string_literals;
    using namespace http;

    std::string data{auth.data.begin(), auth.data.end()};

    printf("Signature:\n%s\n", Botan::base64_encode(auth.signature).c_str());

    // Setup headers
    const Header_set headers{
      { header::Content_Type, "application/json" },
      { header::Accept, "application/json" },
      { "X-MEN-Signature", Botan::base64_encode(auth.signature) }
    };

    // Make post
    httpclient_->post(cached_, "/api/devices/0.1/authentication/auth_requests", headers, {data.begin(), data.end()},
    [](auto err, auto res)
    {
      if(!res)
        printf("No reply.\n");
      else
        printf("Reply:\n%s\n", res->to_string().c_str());
    });
  }

}

#endif
