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

#include <os>
#include <plugins/unik.hpp>
#include <net/interfaces>

void Service::start(const std::string&)
{
  INFO("Unik init test", "Testing unik plugin initialization");

  unik::Client::on_registered([]{
      INFO("Unik test", "Instance registered OK");
      INFO("Unik test", "SUCCESS");
    });

  static auto& inet = net::Interfaces::get(0);
  inet.negotiate_dhcp(5.0, [](auto timeout){
      CHECK(true, "A service can subscribe to the DHCP event even if Unik did so first");
      if (timeout) {
        INFO("Unik test", "DHCP timed out");
        CHECKSERT(not inet.udp().is_bound(unik::default_port), "Unik UDP port is free as expected");

        INFO("Unik test", "Manual netwok config");
        inet.network_config({10,0,0,56},{255,255,255,0},{10,0,0,1},{8,8,8,8});
        unik::Client::register_instance(inet);

      } else {
        INFO("Unik test", "DHCP OK. We can now use the IP stack");
        CHECK(inet.udp().is_bound(unik::default_port), "Unik UDP port is bound as expected");
      }
      try {
        inet.udp().bind(unik::default_port);
      } catch(net::UDP::Port_in_use_exception& e){
        CHECK(true, "Trying to bound to the Unik port now fails");
        INFO("Unik test", "SUCCESS");
      }
    });
}
