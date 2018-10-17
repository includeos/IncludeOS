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

#pragma once
#ifndef NET_VLAN_MANAGER_HPP
#define NET_VLAN_MANAGER_HPP

#include "vif.hpp"
#include <net/ethernet/ethernet_8021q.hpp>

namespace net {

/**
 * @brief      Responsible for redirecting packets to the correct VLAN interface.
 *
 * @note       Currently singleton, which means the ID's are globaly unique.
 */
class VLAN_manager {
public:
  using VLAN_interface = Vif<Ethernet_8021Q>;

  /**
   * @brief      Returns a VLAN manager with the given index.
   *             Construct it if it do not already exist.
   *
   * @param[in]  N     Index
   *
   * @return     A VLAN Manager
   */
  static VLAN_manager& get(int N);

  /**
   * @brief      Add a VLAN interface on the given (physical) link
   *             with a given ID.
   *
   * @param      link  The link
   * @param[in]  id    The identifier
   *
   * @return     A newly created VLAN interface
   */
  VLAN_interface& add(hw::Nic& link, const int id);

private:
  std::map<int, VLAN_interface*> links_;

  VLAN_manager() = default;

  /**
   * @brief      Receive a packet
   *
   * @param[in]  pkt   The packet
   */
  void receive(Packet_ptr pkt);

  /**
   * @brief      Set the vlan upstream on the physical Nic
   *             to point on this manager
   *
   * @param      link  The link
   */
  void setup(hw::Nic& link,const int id)
  { link.set_vlan_upstream({this, &VLAN_manager::receive});
    link.add_vlan(id);
  }

};

} // < namespace net

#endif
