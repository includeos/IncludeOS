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
    using Stack = IP6::Stack;
    using config_func = delegate<void(bool, IP6::addr)>;

    Slaac() = delete;
    Slaac(Slaac&) = delete;
    Slaac(Stack& inet);

    // autoconfigure linklocal and global address
    void autoconf(uint32_t timeout_secs) {}

    // Signal indicating the result of DHCP negotation
    // timeout is true if the negotiation timed out
    void on_config(config_func handler) {}
  private:
    bool flags;
  };
}

#endif
