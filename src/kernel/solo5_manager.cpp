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

#include <assert.h>
#include <common>
#include <delegate>
#include <vector>

#include <kernel/solo5_manager.hpp>
#include <stdexcept>

using namespace hw;
using Nic_ptr = std::unique_ptr<hw::Nic>;
using Blk_ptr = std::unique_ptr<hw::Block_device>;

static std::vector<delegate<Nic_ptr()>> nics;
static std::vector<delegate<Blk_ptr()>> blks;

void Solo5_manager::register_net(delegate<Nic_ptr()> func)
{
  nics.push_back(func);
}
void Solo5_manager::register_blk(delegate<Blk_ptr()> func)
{
  blks.push_back(func);
}

void Solo5_manager::init() {
  INFO("Solo5", "Looking for solo5 devices");

  for (auto nic : nics)
    hw::Devices::register_device<hw::Nic> (nic());
  for (auto blk : blks)
    hw::Devices::register_device<hw::Block_device> (blk());
}
