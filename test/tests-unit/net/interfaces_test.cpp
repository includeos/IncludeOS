// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 Oslo and Akershus University College of Applied Sciences
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

#include <nic_mock.hpp>
#include <hal/machine.hpp>
#include <net/interfaces.hpp>

using namespace net;

CASE("Interfaces functionality")
{
  bool stack_not_found = false;
  bool stack_err = false;

  // Add 3 nics
  os::machine().add<hw::Nic>(std::make_unique<Nic_mock>());
  os::machine().add<hw::Nic>(std::make_unique<Nic_mock>());
  os::machine().add<hw::Nic>(std::make_unique<Nic_mock>());

  auto nics = os::machine().get<hw::Nic>();

  // 3 stacks are preallocated
  EXPECT(Interfaces::get().size() == 3);

  // Retreiving the first stack creates an interface on the first nic
  auto& stack1 = Interfaces::get(0);
  EXPECT(stack1.nic().mac() == nics.at(0).get().mac());

  // Trying to get a stack that do not exists will throw
  stack_not_found = false;
  try {
    Interfaces::get(3);
  } catch(const Stack_not_found&) {
    stack_not_found = true;
  }
  EXPECT(stack_not_found == true);

  // Getting by mac addr works
  const MAC::Addr my_mac{0xff, 0xff, 0xff, 0xff, 0xff, 0xff};
  // hehe..
  reinterpret_cast<Nic_mock&>(nics[0].get()).mac_ = my_mac;
  auto& stack_by_mac = Interfaces::get(my_mac);
  EXPECT(stack_by_mac.nic().mac() == nics.at(0).get().mac());

  // Throws if mac addr isnt found
  stack_not_found = false;
  try {
    Interfaces::get("FF:FF:FF:00:00:00");
  } catch(const Interfaces_err&) {
    stack_not_found = true;
  }
  EXPECT(stack_not_found == true);

  // Creating substacks works alrite
  Nic_mock my_nic;
  auto& my_sub_stack = Interfaces::create(my_nic, 2, 42);
  EXPECT(&my_sub_stack == &Interfaces::get(2,42));

  // Not allowed to create if already occupied tho
  stack_err = false;
  try {
    Interfaces::create(my_nic, 0, 0);
  } catch(const Interfaces_err&) {
    stack_err = true;
  }
  EXPECT(stack_err == true);

}

