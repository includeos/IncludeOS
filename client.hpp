
#pragma once
#ifndef MENDER_CLIENT_HPP
#define MENDER_CLIENT_HPP

#include "auth_manager.hpp"
#include <net/inet4>
#include <string>
#include <botan/base64.h>
#include <net/http/client.hpp>

namespace mender {

  static const std::string api_prefx = "/api/devices/0.1/";

	class Client {
  public:
    using AuthCallback  = delegate<void(bool)>;
  public:
    Client(Auth_manager&&, const std::string& server, const uint16_t port = 0);
    Client(Auth_manager&&, net::tcp::Socket);
    void make_auth_request(AuthCallback);

  private:
    const std::string server_;
    Auth_manager am_;
    net::tcp::Socket cached_;
    std::unique_ptr<http::Client> httpclient_;

    std::string build_url(const std::string& server) const;

    std::string build_api_url(const std::string& server, const std::string& url) const;

    void send(std::unique_ptr<http::Request> req, std::string endpoint);

  }; // < class Client

  Client::Client(Auth_manager&& man, const std::string& server, const uint16_t port)
    : server_(server),
      am_{std::forward<Auth_manager>(man)},
      cached_{0, port}
  {
    static auto&& stack = net::Inet4::ifconfig(
      { 10,0,0,42 },     // IP
      { 255,255,255,0 }, // Netmask
      { 10,0,0,1 },      // Gateway
      { 10,0,0,1 });     // DNS);

    httpclient_ = std::make_unique<http::Client>(stack.tcp());
  }

  Client::Client(Auth_manager&& man, net::tcp::Socket socket)
    : server_{socket.address().to_string()},
      am_{std::forward<Auth_manager>(man)},
      cached_(std::move(socket))
  {
    static auto&& stack = net::Inet4::ifconfig(
      { 10,0,0,42 },     // IP
      { 255,255,255,0 }, // Netmask
      { 10,0,0,1 },      // Gateway
      { 10,0,0,1 });     // DNS);

    httpclient_ = std::make_unique<http::Client>(stack.tcp());
  }

  void Client::make_auth_request(AuthCallback cb)
  {
    auto auth = am_.make_auth_request();

    using namespace std::string_literals;
    using namespace http;

    std::string data{auth.data.begin(), auth.data.end()};

    auto req = httpclient_->create_request();
    req->set_method(POST);
    req->set_uri(URI{"/api/devices/0.1/authentication/auth_requests"});

    auto& header = req->header();
    header.add_field(header::Content_Type, "application/json");
    header.add_field(header::Authorization, "Bearer " + std::string{auth.token.begin(), auth.token.end()} );
    header.add_field("X-MEN-Signature", std::string{Botan::base64_encode(auth.signature)});
    req->add_body(std::string{data.begin(), data.end()});

    printf("Signature:\n%s\n", Botan::base64_encode(auth.signature).c_str());

    httpclient_->send(std::move(req), cached_,
      [](auto err, http::Response_ptr res)
      {
        if(!res)
          printf("No reply.\n");
        else
          printf("Reply:\n%s", res->to_string().c_str());

      }
    );
  }

  void Client::send(std::unique_ptr<http::Request> req, std::string endpoint)
  {
    static auto&& stack = net::Inet4::stack();

    if(cached_.address() == 0)
    {
      /*stack.resolve(server_,
      [this, endpoint, req_{std::move(req)}] // move into capture is c++14
      (auto addr)
      {
        //assert(addr != 0);
        cached_ = {addr, cached_.port()};
        if(addr != 0)
          send(std::move(req_), endpoint);
      });*/
      return;
    }

    stack.tcp().connect(cached_,
    [this, req{std::move(req)}] (auto conn)
    {
      conn->on_read(2048, [](auto, auto){});
      conn->write(req->to_string());
    });

  }

}

#endif
