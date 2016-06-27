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
#include <vga>
#include <cassert>
#include <net/inet4>
#include <hw/ps2.hpp>

#include "snake.hpp"

std::unique_ptr<net::Inet4<VirtioNet> > inet;
ConsoleVGA vga;


void begin_snake()
{
  hw::KBM::init();
  static Snake snake(vga);

  hw::KBM::set_virtualkey_handler(
  [] (int key) {

    if (key == hw::KBM::VK_RIGHT) {
      snake.set_dir(Snake::RIGHT);
    }
    if (key == hw::KBM::VK_LEFT) {
      snake.set_dir(Snake::LEFT);
    }
    if (key == hw::KBM::VK_UP) {
      snake.set_dir(Snake::UP);
    }
    if (key == hw::KBM::VK_DOWN) {
      snake.set_dir(Snake::DOWN);
    }
    if (key == hw::KBM::VK_SPACE) {
      if (snake.is_gameover())
        snake.reset();
    }

  });
}

void Service::start()
{
  // redirect stdout to vga screen
  // ... even though we aren't really using it after the game starts

  OS::set_rsprint(
  [] (const char* data, size_t len)
  {
    vga.write(data, len);
  });

  // we have to start snake later to avoid late text output
  hw::PIT::on_timeout(0.25, [] { begin_snake(); });

  // boilerplate
  hw::Nic<VirtioNet>& eth0 = hw::Dev::eth<0,VirtioNet>();
  inet = std::make_unique<net::Inet4<VirtioNet> >(eth0, 0.15);
  inet->network_config(
    { 10,0,0,42 },      // IP
    { 255,255,255,0 },  // Netmask
    { 10,0,0,1 },       // Gateway
    { 8,8,8,8 } );      // DNS

  printf("*** TEST SERVICE STARTED *** \n");
}
