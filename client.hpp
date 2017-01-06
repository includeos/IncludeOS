
#pragma once
#ifndef MENDER_CLIENT_HPP
#define MENDER_CLIENT_HPP

#include "auth_manager.hpp"
#include <net/tcp/tcp.hpp>
#include <string>
#include <net/http/client.hpp>
#include <timers>
#include "state.hpp"

namespace mender {

  static const std::string api_prefx = "/api/devices/0.1/";

	class Client {
  public:
    using AuthCallback  = delegate<void(bool)>;
  public:
    Client(Auth_manager&&, net::TCP&, const std::string& server, const uint16_t port = 0);
    Client(Auth_manager&&, net::TCP&, net::tcp::Socket);

    void make_auth_request();

    void check_for_update();

    void on_auth(AuthCallback cb)
    { on_auth_ = cb; }

    bool is_authed() const
    { return !am_.auth_token().empty(); }

    void authenticate(Timers::duration_t interval, AuthCallback cb = nullptr);

  private:
    // auth related
    Auth_manager am_;
    AuthCallback on_auth_;

    // http related
    const std::string server_;
    net::tcp::Socket cached_;
    std::unique_ptr<http::Client> httpclient_;

    // state related
    friend class state::State;
    state::State* state_;
    state::Context context_;


    std::string build_url(const std::string& server) const;

    std::string build_api_url(const std::string& server, const std::string& url) const;

    http::Header_set create_headers(const byte_seq& signature) const;

    void response_handler(http::Error err, http::Response_ptr res);

    void auth_handler(http::Error err, http::Response_ptr res);

    bool is_valid_response(const http::Response& res) const;

    void auth_success(const http::Response& res);

    void update_handler(http::Error err, http::Response_ptr res);

    void run_state()
    {
      switch(state_->handle(*this, context_))
      {
        using namespace state;
        case State::Result::GO_NEXT:
          run_state();
          return;

        case State::Result::DELAYED_NEXT:
          run_state_delayed();
          return;

        case State::Result::AWAIT_EVENT:
          // setup timeout
          return;
      }
    }
    /*void state_handler()
    {
      if(!context_.delay)
        run_state_loop();
      else {

      }
    }

    void run_state_loop()
    {
      while(run_state());
    }

    bool run_state()
    { return state_->handle(*this, context_); }*/

    void run_state_delayed()
    { context_.timer.start(std::chrono::seconds(context_.delay), {this, &Client::run_state}); }

    void set_state(state::State& s)
    { state_ = &s; }

  }; // < class Client

}

#endif
