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

//#define VLAN_DEBUG 1
#ifdef VLAN_DEBUG
#define PRINT(fmt, ...) printf(fmt, ##__VA_ARGS__)
#else
#define PRINT(fmt, ...) /* fmt */
#endif

#include <net/vlan_manager.hpp>
#include <hal/machine.hpp>

namespace net {


VLAN_manager& VLAN_manager::get(int N)
{
  static std::map<int, std::unique_ptr<VLAN_manager>> managers;
  auto it = managers.find(N);
  if(it != managers.end())
  {
    return *it->second;
  }
  else
  {
    PRINT("<VLAN_manager> Creating VLAN manager for N=%i\n", N);
    auto ptr = std::unique_ptr<VLAN_manager>(new VLAN_manager);
    auto ok = managers.emplace(N, std::move(ptr));
    Expects(ok.second);
    return *ok.first->second;
  }
}

VLAN_manager::VLAN_interface& VLAN_manager::add(hw::Nic& link, const int id)
{
  Expects(id > 0 and id <= 4096 && "Outside VID range (1-4096)");

  // this is very redudant if it's already been set once,
  // but i'll keep it for now since it's not expensive.
  this->setup(link,id);

  auto vif = std::make_unique<VLAN_interface>(link, id);
  auto* raw = vif.get();

  // register as a device (unnecessary?)
  os::machine().add<hw::Nic>(std::move(vif));

  auto it = links_.emplace(id, raw);
  Ensures(it.second && "Could not insert - ID is most likely already taken");

  INFO("VLAN", "Added VLAN %s %s", raw->driver_name(), raw->device_name().c_str());

  return *it.first->second;
}

void VLAN_manager::receive(Packet_ptr pkt)
{
  auto& vlan = *reinterpret_cast<ethernet::VLAN_header*>(pkt->layer_begin());
  Expects(vlan.tpid == static_cast<int>(Ethertype::VLAN));

  auto id = vlan.vid();

  auto it = links_.find(id);
  PRINT("<VLAN_manager> Recieved frame tagged with ID %d ", id);
  if(it != links_.end())
  {
    PRINT("- Found\n");
    it->second->receive(std::move(pkt));
    return;
  }
  else
  {
    PRINT("- Not found\n");
  }
}

} // < namespace net
