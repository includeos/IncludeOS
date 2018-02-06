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

#include <service>
#include <timers>
#include <hw/vga_gfx.hpp>
#include <array>
#include <cassert>
#include <cmath>
#include <deque>
#include <x86intrin.h>
using namespace std::chrono;

static uint8_t backbuffer[320*200] __attribute__((aligned(16)));
inline void set_pixel(int x, int y, uint8_t cl)
{
  //assert(x >= 0 && x < 320 && y >= 0 && y < 200);
  if (x >= 0 && x < 320 && y >= 0 && y < 200)
      backbuffer[y * 320 + x] = cl;
}
inline void clear()
{
  memset(backbuffer, 0, sizeof(backbuffer));
}

static int timev = 0;

struct Star
{
  Star() {
    x = rand() % 320;
    y = rand() % 200;
    cl = 18 + rand() % 14;
  }
  void render()
  {
    uint8_t dark = std::max(cl - 6, 16);
    set_pixel(x+1, y, dark);
    set_pixel(x-1, y, dark);
    set_pixel(x, y+1, dark);
    set_pixel(x, y-1, dark);
    set_pixel(x, y, cl);
  }
  void modulate()
  {
    clear();
    if (cl == 16) return;

    float dx = (x - 160);
    float dy = (y - 100);
    if (dx == 0 && dy == 0) return;

    float mag = 1.0f / sqrtf(dx*dx + dy*dy);
    dx *= mag;
    dy *= mag;
    x += dx * 1.0f;
    y += dy * 1.0f;

    render();
  }
  void clear()
  {
    set_pixel(x+1, y, 0);
    set_pixel(x-1, y, 0);
    set_pixel(x, y+1, 0);
    set_pixel(x, y-1, 0);
    set_pixel(x, y, 0);
  }

  float x, y;
  uint8_t cl;
};
static std::deque<Star> stars;

void Service::start()
{
  VGA_gfx::set_mode(VGA_gfx::MODE_320_200_256);
  VGA_gfx::clear();
  VGA_gfx::apply_default_palette();

  clear();

  Timers::periodic(16ms,
  [] (int) {
    timev++;
    // add new (random) star
    if (rand() % 2 == 0)
    {
      Star star;
      star.render();
      stars.push_back(star);
    }
    // render screen
    VGA_gfx::blit_from(backbuffer);

    // work on backbuffer
    for (auto& star : stars)
    {
      star.modulate();
    }

    if (stars.size() > 50)
    {
      auto& dead_star = stars.front();
      dead_star.clear();
      stars.pop_front();
    }
  });
}
