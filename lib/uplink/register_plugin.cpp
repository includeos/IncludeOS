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

#include "uplink.hpp"
#include "common.hpp"
#include <kernel/os.hpp>
#include <os.hpp>

static void setup_uplink_plugin()
{
  try
  {
    uplink::get();
  }
  catch(const std::exception& e)
  {
    MYINFO("Rebooting");
    os::reboot();
  }
}

__attribute__((constructor))
void register_plugin_uplink(){
  OS::register_plugin(setup_uplink_plugin, "Uplink");
}
