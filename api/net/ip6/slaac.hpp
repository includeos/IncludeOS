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
#ifndef NET_SLAAC_HPP
#define NET_SLAAC_HPP

#include <util/timer.hpp>
#include "ip6.hpp"
#include <net/ip6/ndp/options.hpp>
#include "stateful_addr.hpp"

namespace net {

  class Slaac
  {
  public:
    static const int LINKLOCAL_RETRIES = 1;
    static const int LINKLOCAL_INTERVAL = 1;
    static const int GLOBAL_RETRIES = LINKLOCAL_RETRIES;
    static const int GLOBAL_INTERVAL = 3;

    using Stack = IP6::Stack;
    using config_func = delegate<void(bool)>;

    Slaac() = delete;
    Slaac(Slaac&) = delete;
    Slaac(Stack& inet);

    // autoconfigure linklocal and global address
    void autoconf_start(int retries, uint64_t token, bool use_token);
    void autoconf_linklocal();
    void autoconf_global_start();
    void autoconf_global();
    void autoconf_trigger();
    void on_config(config_func handler);

  private:
    Stack&        stack;
    uint64_t      token_;
    bool          use_token_;
    ip6::Stateful_addr tentative_addr_;
    bool          linklocal_completed;
    // Number of times to attempt DAD
    int           dad_transmits_;
    Timer         timeout_timer_;
    std::vector<config_func> config_handlers_;
    std::chrono::milliseconds interval;

    void process_prefix_info(const ndp::option::Prefix_info& pinfo);
    void perform_dad();
    void dad_handler(const ip6::Addr& addr);
  };
}

#endif
