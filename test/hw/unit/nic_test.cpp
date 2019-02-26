// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016-2017 Oslo and Akershus University College of Applied Sciences
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

#include <common.cxx>
#include <hw/devices.hpp>
#include <hw/nic.hpp>
#include <nic_mock.hpp>

CASE("Register network device")
{
  EXPECT(hw::Devices::device_next_index<hw::Nic> () == 0);
  EXPECT_THROWS(hw::Devices::get<hw::Nic> (0));
  hw::Devices::register_device<hw::Nic> (std::make_unique<Nic_mock>());
  
  EXPECT(hw::Devices::nic(0).driver_name() == std::string("Mock driver"));
  // flush all devices
  hw::Devices::flush_all();
  // deactivate all devices
  hw::Devices::deactivate_all();
}

CASE("NIC interface")
{
  EXPECT(hw::Nic::device_type() == "NIC");
  Nic_mock nic;
  // add vlan?
  nic.add_vlan(0);
}
