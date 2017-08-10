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

#include "ws_uplink.hpp"
#include "common.hpp"
#include <os>

namespace uplink {

static std::unique_ptr<WS_uplink> uplink{nullptr};


  void on_panic(const char* why){
    if (uplink)
      uplink->panic(why);
  }


void setup_uplink()
{
  MYINFO("Setting up WS uplink");

  try {
    auto& en0 = net::Super_stack::get<net::IP4>(0);

    uplink = std::make_unique<WS_uplink>(en0);

    OS::on_panic(uplink::on_panic);

  }catch(const std::exception& e) {
    MYINFO("Uplink initialization failed: %s ", e.what());
    MYINFO("Rebooting");
    OS::reboot();
  }
}

} // < namespace uplink

#include <kernel/os.hpp>
__attribute__((constructor))
void register_plugin_uplink(){
  OS::register_plugin(uplink::setup_uplink, "Uplink");
}
