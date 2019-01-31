// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2017 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_DHCP_DH4CLIENT_HPP
#define NET_DHCP_DH4CLIENT_HPP

#include "dhcp4.hpp"
#include "options.hpp"

#include <util/timer.hpp>
#include <net/udp/udp.hpp>

namespace net {

  class DHClient
  {
  public:
    static constexpr std::chrono::seconds RETRY_FREQUENCY{1};
    static constexpr std::chrono::seconds RETRY_FREQUENCY_SLOW{10};

    using Stack = Inet;
    using config_func = delegate<void(bool)>;

    DHClient() = delete;
    DHClient(DHClient&) = delete;
    DHClient(Stack& inet);

    // negotiate with local DHCP server
    void negotiate(std::chrono::seconds timeout = std::chrono::seconds::zero());

    // Signal indicating the result of DHCP negotation
    // timeout is true if the negotiation timed out
    void on_config(config_func handler);

  private:
    void send_first();
    void offer(const char* data, size_t len);
    void request(const dhcp::option::server_identifier* server_id);   // --> acknowledge
    void acknowledge(const char* data, size_t len);

    void restart_negotation();
    void end_negotiation(bool);

    Stack& stack;
    uint32_t     xid = 0;
    ip4::Addr    ipaddr, netmask, router, dns_server;
    std::string  domain_name;
    uint32_t     lease_time;
    std::vector<config_func> config_handlers_;
    int          tries    = 0;
    int          progress = 0;
    Timer        timeout_timer_;
    std::chrono::milliseconds timeout;
    udp::Socket* socket = nullptr;
  };

}

#endif
