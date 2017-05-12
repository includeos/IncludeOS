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

#include <net/autoconf.hpp>

namespace uplink {

static std::unique_ptr<WS_uplink> uplink{nullptr};

void setup_uplink()
{
  MYINFO("Setting up WS uplink");

  net::autoconf::load();

  auto& en0 = net::Super_stack::get<net::IP4>(0);
  
  // already initialized
  if(en0.ip_addr() != 0)
  {
    uplink = std::make_unique<WS_uplink>(en0);
  }
  // if not register on config event
  else
  {
    en0.on_config(
    [] (bool timeout)
    {
      auto& en0 = net::Super_stack::get<net::IP4>(0);
      if(timeout)
      {
        // temporary fallback
        en0.network_config(
          { 10,0,0,42 },     // IP
          { 255,255,255,0 }, // Netmask
          { 10,0,0,1 },      // Gateway
          { 10,0,0,1 });     // DNS
      }

      uplink = std::make_unique<WS_uplink>(en0);
    });
  }
}

} // < namespace uplink

#include <kernel/os.hpp>
__attribute__((constructor))
void register_plugin_uplink(){
  OS::register_plugin(uplink::setup_uplink, "Uplink");
}
