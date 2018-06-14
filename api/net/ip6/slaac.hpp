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

namespace net {

  class Slaac
  {
  public:
    static const int NUM_RETRIES = 1;
    static const int INTERVAL = 1;
    static const int MAX_RTR_SOLICITATIONS = 5;
    static const int RTR_SOLICITATION_INTERVAL = 5;

    using Stack = IP6::Stack;
    using config_func = delegate<void(bool)>;

    Slaac() = delete;
    Slaac(Slaac&) = delete;
    Slaac(Stack& inet);

    // autoconfigure linklocal and global address
    void autoconf_start(int retries,
            IP6::addr alternate_addr = IP6::ADDR_ANY);
    void autoconf();
    void autoconf_trigger();
    void on_config(config_func handler);

  private:
    Stack& stack;
    IP6::addr    alternate_addr_;
    IP6::addr    tentative_addr_;
    uint32_t     lease_time;
    // Number of times to attempt DAD
    int          dad_retransmits_ = NUM_RETRIES;
    int          progress = 0;
    Timer        timeout_timer_;
    std::vector<config_func> config_handlers_;
    std::chrono::milliseconds interval;
  };
}

#endif
