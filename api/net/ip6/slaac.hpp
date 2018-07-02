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

namespace net {

  class Slaac
  {
  public:
    static const int LINKLOCAL_RETRIES = 1;
    static const int LINKLOCAL_INTERVAL = 1;
    static const int GLOBAL_RETRIES = 5;
    static const int GLOBAL_INTERVAL = 5;

    using Stack = IP6::Stack;
    using config_func = delegate<void(bool)>;

    Slaac() = delete;
    Slaac(Slaac&) = delete;
    Slaac(Stack& inet);

    // autoconfigure linklocal and global address
    void autoconf_start(int retries,
            IP6::addr alternate_addr = IP6::ADDR_ANY);
    void autoconf_linklocal();
    void autoconf_global();
    void autoconf_trigger();
    void on_config(config_func handler);

  private:
    Stack& stack;
    IP6::addr    alternate_addr_;
    IP6::addr    tentative_addr_;
    bool         linklocal_completed;
    // Number of times to attempt DAD
    int          dad_retransmits_;
    Timer        timeout_timer_;
    std::vector<config_func> config_handlers_;
    std::chrono::milliseconds interval;
  };
}

#endif
