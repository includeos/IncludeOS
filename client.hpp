
#pragma once
#ifndef MENDER_CLIENT_HPP
#define MENDER_CLIENT_HPP

#include "auth_manager.hpp"
#include <net/tcp/tcp.hpp>
#include <string>
#include <net/http/client.hpp>
#include <timers>
#include "state.hpp"
#include "device.hpp"
#include "liveupdate.hpp"

namespace mender {

  static const std::string API_PREFIX = "/api/devices/0.1";

	class Client {
  public:
    using AuthCallback  = delegate<void(bool)>;
  public:
    Client(Auth_manager&&, Device&&, net::TCP&, const std::string& server, const uint16_t port = 0);
    Client(Auth_manager&&, Device&&, net::TCP&, net::tcp::Socket);

    void make_auth_request();

    void check_for_update();

    void fetch_update(http::Response_ptr = nullptr);

    void update_inventory_attributes();

    void install_update(http::Response_ptr = nullptr);

    void set_auth_token(Auth_token token)
    { am_.set_auth_token(token); }

    bool is_authed() const
    { return !am_.auth_token().empty(); }

    Device& device()
    { return device_; }

    void boot();

    void resume(state::State& s)
    {
      set_state(s);
      run_state();
    }

  private:
    // auth related
    Auth_manager am_;

    // device
    Device device_;

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

    http::Header_set create_auth_headers(const byte_seq& signature) const;

    void response_handler(http::Error err, http::Response_ptr res);

    bool is_valid_response(const http::Response& res) const;

    bool is_json(const http::Response& res) const;

    bool is_artifact(const http::Response& res) const;

    http::URI parse_update_uri(http::Response& res);

    void store_state(liu::Storage, liu::buffer_len);

    void load_state(liu::Restore);

    void run_state();

    void run_state_delayed()
    { context_.timer.start(std::chrono::seconds(context_.delay), {this, &Client::run_state}); }

    void set_state(state::State& s)
    {
      auto old{state_->to_string()};
      state_ = &s;
      printf("<Client> State transition: %s => %s\n",
          old.c_str(),
          state_->to_string().c_str());
    }

  }; // < class Client

}

#endif
