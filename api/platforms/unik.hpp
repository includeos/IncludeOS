// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#ifndef PLATFORMS_UNIK_HPP
#define PLATFORMS_UNIK_HPP

#include <net/inet4.hpp>

namespace unik{

  const net::UDP::port_t default_port = 9967;

  class Client {
  public:
    using Registered_event = delegate<void()>;

    static void register_instance(net::Inet4& inet, const net::UDP::port_t port = default_port);
    static void register_instance_dhcp();
    static void on_registered(Registered_event e) {
      on_registered_ = e;
    };

  private:
    static Registered_event on_registered_;
  };

}

#endif
