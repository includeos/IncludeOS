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
#include <timers>
#include <vga>
#include <hw/ps2.hpp>

#include "snake.hpp"

ConsoleVGA vga;

void begin_snake()
{
  static Snake snake {vga};

  hw::KBM::init();
  hw::KBM::set_virtualkey_handler(
  [] (int key) {
    if (key == hw::KBM::VK_RIGHT) {
      snake.set_dir(Snake::RIGHT);
    } else if (key == hw::KBM::VK_LEFT) {
      snake.set_dir(Snake::LEFT);
    } else if (key == hw::KBM::VK_UP) {
      snake.set_dir(Snake::UP);
    } else if (key == hw::KBM::VK_DOWN) {
      snake.set_dir(Snake::DOWN);
    } else if (key == hw::KBM::VK_SPACE and snake.is_gameover()) {
      snake.reset();
    }
  });
}

void Service::start(const std::string&)
{
  // We have to start snake later to avoid some text output
  begin_snake();
}
