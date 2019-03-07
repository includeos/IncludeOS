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
#include <hw/usernet.hpp>

CASE("UserNet interface")
{
  auto& nic = UserNet::create(64000);
  EXPECT(nic.device_type() == hw::Device::Type::Nic);
  EXPECT(nic.driver_name() == std::string("UserNet"));
  nic.poll();
  nic.flush();
  nic.move_to_this_cpu();
  nic.deactivate();

  EXPECT(nic.create_physical_downstream() != nullptr);
}
