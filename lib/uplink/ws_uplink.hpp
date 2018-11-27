// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef UPLINK_WS_UPLINK_HPP
#define UPLINK_WS_UPLINK_HPP

#include "transport.hpp"
#include "config.hpp"

#include <net/inet>
#include <net/http/client.hpp>
#include <net/ws/websocket.hpp>
#if defined(LIVEUPDATE)
  #include <liveupdate.hpp>
#endif
#include <util/timer.hpp>
#include <util/logger.hpp>
#include <rtc>

namespace uplink {

class WS_uplink {
public:
  static constexpr auto heartbeat_interval = 10s;
  static constexpr auto heartbeat_retries  = 3;

  WS_uplink(Config config);

  void start(net::Inet&);

  void auth();

  void dock();

  void handle_transport(Transport_ptr);

  void send_ident();

  void send_log(const char*, size_t);

  void flush_log();

  void send_uplink();

  void update(std::vector<char> buffer);

  void send_error(const std::string& err);

  void send_stats();

  void send_message(Transport_code, const char* data, size_t len);

  bool is_online() const
  { return ws_ != nullptr and ws_->is_alive(); }

private:
  Config config_;

  net::Inet&                   inet_;
  std::unique_ptr<http::Basic_client> client_;
  net::WebSocket_ptr            ws_;
  std::string                   id_;
  std::string                   token_;
  /** Hash for the current running binary
   * (restored during update, none if never updated) */
  std::string                   binary_hash_;
  /** Hash for current received update */
  std::string                   update_hash_;

  Transport_parser parser_;

  Timer retry_timer;
  uint8_t retry_backoff = 0;
  uint8_t heart_retries_left = heartbeat_retries;

  std::vector<char> logbuf_;

  Timer heartbeat_timer;
  RTC::timestamp_t last_ping;

  RTC::timestamp_t update_time_taken = 0;

  void inject_token(http::Request& req, http::Basic_client::Options&, const http::Basic_client::Host)
  {
    if (not token_.empty())
      req.header().add_field("Authorization", "Bearer " + token_);
  }

  std::string auth_data() const;

  void handle_auth_response(http::Error err, http::Response_ptr res, http::Connection&);

  void retry_auth();

  void establish_ws(net::WebSocket_ptr ws);

  void handle_ws_close(uint16_t code);

  bool handle_ping(const char*, size_t);
  void handle_pong_timeout(net::WebSocket&);

  bool missing_heartbeat()
  { return last_ping < RTC::now() - heartbeat_interval.count(); }
  void on_heartbeat_timer();

  void parse_transport(net::WebSocket::Message_ptr msg);
#if defined(LIVEUPDATE)
  void store(liu::Storage& store, const liu::buffer_t*);

  void restore(liu::Restore& store);

  void store_conntrack(liu::Storage& store, const liu::buffer_t*);

  void restore_conntrack(liu::Restore& store);
#endif
}; // < class WS_uplink


} // < namespace uplink

#endif
