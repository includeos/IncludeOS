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
#include <hw/mac_addr.hpp>
#include <hw/nic.hpp>
#include <vector>
#include <map>
#include <stdexcept>

namespace net {

struct Super_stack_err : public std::runtime_error {
  using base = std::runtime_error;
  using base::base;
};

struct Stack_not_found : public Super_stack_err
{
  using Super_stack_err::Super_stack_err;
};

class Super_stack {
public:
  // naming is hard...
  using Indexed_stacks = std::map<int, std::unique_ptr<Inet>>;
  using Stacks = std::vector<Indexed_stacks>;

public:
  static Super_stack& inet()
  {
    static Super_stack stack_;
    return stack_;
  }

  static Inet& get(int N);
  static Inet& get(int N, int sub);

  /**
   * @brief      Get a stack by MAC addr.
   *             Throws if no NIC with the given MAC exists.
   *
   * @param[in]  mac   The mac
   *
   * @tparam     IP version
   *
   * @return     A stack
   */
  static Inet& get(const std::string& mac);
  static Inet& get(const std::string& mac, int sub);

  Inet& create(hw::Nic& nic, int N, int sub);

  Stacks& stacks()
  { return stacks_; }

private:
  Stacks stacks_;

  Super_stack();

};

} // < namespace net


#endif
