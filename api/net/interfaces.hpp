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

#include <net/inet.hpp>
#include <vector>
#include <map>
#include <stdexcept>

namespace net {

struct Interfaces_err : public std::runtime_error {
  using base = std::runtime_error;
  using base::base;
};

struct Stack_not_found : public Interfaces_err
{
  using Interfaces_err::Interfaces_err;
};

class Interfaces {
public:
  // naming is hard...
  using Indexed_stacks = std::map<int, std::unique_ptr<Inet>>;
  using Stacks = std::vector<Indexed_stacks>;

public:
  static Interfaces& instance()
  {
    static Interfaces stack_;
    return stack_;
  }

  /**
   * @brief      Get Stack with the given ID
   * @note       Throws if not found
   *
   * @param[in]  N     id
   *
   * @return     Stack with id N
   */
  static Inet& get(int N);
  /**
   * @brief      Get a substack with a given ID and sub ID.
   *             Used for VLAN purposes (0 is always the non-VLAN iface)
   *
   * @param[in]  N     Id
   * @param[in]  sub   The sub
   *
   * @return     Stack with id N and sub index sub
   */
  static Inet& get(int N, int sub);

  /**
   * @brief      Get all them stacks
   *
   * @return     List with Indexed stacks
   */
  static Stacks& get()
  { return instance().stacks_; }

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

  static Inet& create(hw::Nic& nic, int N, int sub);

  // NIC helpers @todo: Maybe move away from Interfaces
  static hw::Nic& get_nic(int idx);
  static hw::Nic& get_nic(const MAC::Addr& mac);
  /**
   * @brief      Gets the NIC index among the machines stored nics.
   *             Negative number if not found.
   *
   * @param[in]  mac   The mac address
   *
   * @return     The nic index.
   */
  static int get_nic_index(const MAC::Addr& mac);

private:
  Stacks stacks_;

  Interfaces();

};

} // < namespace net


#endif
