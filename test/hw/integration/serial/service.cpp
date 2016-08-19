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
#include <hw/serial.hpp>
#include <kernel/irq_manager.hpp>

using namespace std::chrono;

void Service::start(const std::string&)
{

  auto& com1 = hw::Serial::port<1>();

  com1.on_readline([](const std::string& s){
      CHECK(true, "Received: %s", s.c_str());

    });
  //IRQ_manager::cpu(0).enable_irq(4);
  INFO("Serial Test","Doing some serious serial");
  printf("trigger_test_serial_port\n");

  hw::PIT::instance().on_repeated_timeout(3s, []{
    printf("I'm alive\n");
  });

}
