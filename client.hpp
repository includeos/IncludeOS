
#pragma once
#ifndef MENDER_CLIENT_HPP
#define MENDER_CLIENT_HPP

#include "auth_manager.hpp"
#include <net/tcp/tcp.hpp>
#include <string>
#include <botan/base64.h>
#include <net/http/client.hpp>
#include <timers>

namespace mender {

  static const std::string api_prefx = "/api/devices/0.1/";

	class Client {
  public:
    using AuthCallback  = delegate<void(bool)>;
  public:
    Client(Auth_manager&&, net::TCP&, const std::string& server, const uint16_t port = 0);
    Client(Auth_manager&&, net::TCP&, net::tcp::Socket);

    void make_auth_request();

    void on_auth(AuthCallback cb)
    { on_auth_ = cb; }

    bool is_authed() const
    { return !am_.auth_token().empty(); }

    void authenticate(Timers::duration_t interval, AuthCallback cb = nullptr);

  private:
    const std::string server_;
    Auth_manager am_;
    net::tcp::Socket cached_;
    std::unique_ptr<http::Client> httpclient_;
    AuthCallback on_auth_;
    Timers::id_t timer_id_ = Timers::UNUSED_ID;

    std::string build_url(const std::string& server) const;

    std::string build_api_url(const std::string& server, const std::string& url) const;

    http::Header_set create_headers(const byte_seq& signature) const;

    void auth_handler(http::Error err, http::Response_ptr res);

    bool is_valid_response(const http::Response& res) const;

    void auth_success(const http::Response& res);

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

  void Client::make_auth_request()
  {
    auto auth = am_.make_auth_request();

    using namespace std::string_literals;
    using namespace http;

    std::string data{auth.data.begin(), auth.data.end()};

    //printf("Signature:\n%s\n", Botan::base64_encode(auth.signature).c_str());

    // Setup headers
    const Header_set headers{
      { header::Content_Type, "application/json" },
      { header::Accept, "application/json" },
      { "X-MEN-Signature", Botan::base64_encode(auth.signature) }
    };

    printf("<Client> POSTing auth request\n");
    // Make post
    httpclient_->post(cached_,
      "/api/devices/0.1/authentication/auth_requests",
      create_headers(auth.signature),
      {data.begin(), data.end()},
      {this, &Client::auth_handler});
  }

  http::Header_set Client::create_headers(const byte_seq& signature) const
  {
    return {
      { http::header::Content_Type, "application/json" },
      { http::header::Accept, "application/json" },
      { "X-MEN-Signature", Botan::base64_encode(signature) }
    };
  }

  void Client::auth_handler(http::Error err, http::Response_ptr res)
  {
    if(!res)
    {
      printf("No reply.\n");
    }
    else
    {
      if(is_valid_response(*res))
      {
        switch(res->status_code())
        {
          case 200:
            printf("<Client> 200 OK:\n");
            auth_success(*res);
            break;

          case 401:
            printf("<Client> 401:\n%s\n", res->body().to_string().c_str());
            break;

          default:
            printf("<Client> Failed with error code: %u\n", res->status_code());
        }
      }
      else
        printf("<Client> Invalid response:\n%s\n", res->to_string().c_str());
    }
  }

  bool Client::is_valid_response(const http::Response& res) const
  {
    const bool is_json = res.header().value(http::header::Content_Type).find("application/json") != std::string::npos;
    return is_json;
  }

  void Client::auth_success(const http::Response& res)
  {
    printf("%s\n", res.to_string().c_str());
    const auto body{res.body().to_string()};
    am_.set_auth_token({body.begin(), body.end()});

    if(on_auth_)
      on_auth_(is_authed());

    if(timer_id_ != Timers::UNUSED_ID) {
      Timers::stop(timer_id_);
      timer_id_ = Timers::UNUSED_ID;
    }
  }

  void Client::authenticate(Timers::duration_t interval, AuthCallback cb)
  {
    on_auth(std::move(cb));

    using namespace std::chrono;
    timer_id_ = Timers::periodic(1s, interval,
    [this](auto)
    {
      make_auth_request();
    });
  }

}

#endif
