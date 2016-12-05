
#pragma once
#ifndef MENDER_CLIENT_HPP
#define MENDER_CLIENT_HPP

#include "auth_manager.hpp"
#include <net/inet4>
#include "../http_client/http/inc/request.hpp"
#include <string>

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

    std::string build_url(const std::string& server) const;

    std::string build_api_url(const std::string& server, const std::string& url) const;

    void send(std::unique_ptr<http::Request> req, std::string endpoint);

  }; // < class Client

  Client::Client(Auth_manager&& man, const std::string& server, const uint16_t port)
    : server_(server),
      am_{std::forward<Auth_manager>(man)},
      cached_{0, port}
  {

  }

  Client::Client(Auth_manager&& man, net::tcp::Socket socket)
    : server_{socket.address().to_string()},
      am_{std::forward<Auth_manager>(man)},
      cached_(std::move(socket))
  {

  }

  byte_seq base64(byte_seq data)
  {
    return data;
  }

  void Client::make_auth_request(AuthCallback cb)
  {
    auto auth = am_.make_auth_request();

    using namespace std::string_literals;
    /*auto req = std::make_unique<http::Request>();
    req->add_body(auth.data);
    req->add_header("Content-Type"s, "application/json"s);
    req->add_header("Authorization"s, "Bearer " + auth.token);
    req->add_header("X-MEN-Signature"s, base64(auth.signature));

    send(std::move(req), "/authentication/auth_requests");*/
    //req.add_header("X-MEN-Signature", signature(auth));
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
