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

#ifndef PLUGINS_NACL_HPP
#define PLUGINS_NACL_HPP

#include <net/ip4/ip4.hpp>

namespace nacl {
  class Filter {
  public:
    virtual net::Filter_verdict<net::IP4> operator()(net::IP4::IP_packet_ptr pckt, net::Inet<net::IP4>& stack, net::Conntrack::Entry_ptr ct_entry) = 0;
  };
}

#endif
