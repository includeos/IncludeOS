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

#ifndef KERNEL_SOLO5_MANAGER_HPP
#define KERNEL_SOLO5_MANAGER_HPP

#include <memory>
#include <delegate>
#include <hw/nic.hpp>
#include <hw/block_device.hpp>

class Solo5_manager {
public:
  using Nic_ptr = std::unique_ptr<hw::Nic>;
  using Blk_ptr = std::unique_ptr<hw::Block_device>;

  static void register_net(delegate<Nic_ptr()>);
  static void register_blk(delegate<Blk_ptr()>);

  static void init();
}; //< class Solo5_manager

#endif //< KERNEL_SOLO5_MANAGER_HPP
