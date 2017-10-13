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
#include <net/ip4/udp.hpp>

namespace net {

  class DHClient
  {
  public:
    static const int NUM_RETRIES = 5;

    using Stack = IP4::Stack;
    using config_func = delegate<void(bool)>;

    DHClient() = delete;
    DHClient(DHClient&) = delete;
    DHClient(Stack& inet);

    // negotiate with local DHCP server
    void negotiate(uint32_t timeout_secs);

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
    IP4::addr    ipaddr, netmask, router, dns_server;
    std::string  domain_name;
    uint32_t     lease_time;
    std::vector<config_func> config_handlers_;
    int          retries  = 0;
    int          progress = 0;
    Timer        timeout_timer_;
    std::chrono::milliseconds timeout;
    UDPSocket* socket = nullptr;
  };

}

#endif
