// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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
#ifndef NET_SUPER_STACK_HPP
#define NET_SUPER_STACK_HPP

#include <net/ip4/ip4.hpp>
#include "inet.hpp"
#include <vector>

namespace net {

class Super_stack {
public:
  using IP4_stack = Inet<IP4>;

public:
  static Super_stack& inet()
  {
    static Super_stack stack_;
    return stack_;
  }

  template <typename IPV>
  static Inet<IPV>& get(int N);

  auto&& ip4_stacks()
  { return ip4_stacks_; }

private:
  std::vector<std::unique_ptr<IP4_stack>> ip4_stacks_;

  Super_stack();

};

} // < namespace net


#endif
