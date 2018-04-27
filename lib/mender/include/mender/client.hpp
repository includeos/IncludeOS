// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#ifndef MENDER_CLIENT_HPP
#define MENDER_CLIENT_HPP

#include "auth_manager.hpp"
#include <net/tcp/tcp.hpp>
#include <string>
#include <net/http/basic_client.hpp>
#include <timers>
#include "state.hpp"
#include "device.hpp"
#include <liveupdate>

namespace mender {

  static const std::string API_PREFIX = "/api/devices/v1";

	class Client {
  public:
    using On_store  = delegate<void(liu::Storage&)>;
    using On_resume = delegate<void(liu::Restore&)>;

  public:
    std::chrono::seconds update_poll_interval{10};

    Client(Auth_manager&&, Device&&, net::TCP&, const std::string& server, const uint16_t port = 0);
    Client(Auth_manager&&, Device&&, net::TCP&, net::Socket);

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

    std::string artifact_name()
    { return device_.inventory().value("artifact_name"); }

    /**
     * @brief      Start the client
     */
    void boot();

    /**
     * @brief      Resume the client starting in the given State
     *
     * @param      s     State to start in
     */
    void resume(state::State& s)
    {
      set_state(s);
      run_state();
    }

    /**
     * @brief      Custom state store
     *
     * @warning    Don't use id's below 10 to not overwrite Client state.
     *
     * @param[in]  func  On store function
     */
    void on_store(On_store func)
    { on_state_store_ = func; }

    /**
     * @brief      Custom state resume
     *
     * @param[in]  func  On resume function
     */
    void on_resume(On_resume func)
    { on_state_resume_ = func; }

  private:
    // auth related
    Auth_manager am_;

    // device
    Device device_;

    // http related
    const std::string server_;
    net::Socket cached_;
    std::unique_ptr<http::Basic_client> httpclient_;

    // state related
    friend class state::State;
    state::State* state_;
    state::Context context_;

    // custom user store/restore
    On_store  on_state_store_;
    On_resume on_state_resume_;

    std::string build_url(const std::string& server) const;

    std::string build_api_url(const std::string& server, const std::string& url) const;

    http::Header_set create_auth_headers(const byte_seq& signature) const;

    void response_handler(http::Error err, http::Response_ptr res, http::Connection&);

    bool is_valid_response(const http::Response& res) const;

    bool is_json(const http::Response& res) const;

    bool is_artifact(const http::Response& res) const;

    http::URI parse_update_uri(http::Response& res);

    void store_state(liu::Storage&, const liu::buffer_t*);

    void load_state(liu::Restore&);

    void run_state();

    void run_state_delayed()
    { context_.timer.restart(std::chrono::seconds(context_.delay)); }

    void set_state(state::State& s)
    {
      const auto old{state_->to_string()};
      state_ = &s;
      MENDER_INFO("Client", "State transition: %s => %s",
          old.c_str(),
          state_->to_string().c_str());
    }

  }; // < class Client

}

#endif
