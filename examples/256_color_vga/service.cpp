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
#include <cmath>
#include <hw/ps2.hpp>
#include <hw/vga_gfx.hpp>

template<typename T>
struct Point
{
  T x;
  T y;
};

struct Color
{
  double r;
  double g;
  double b;
};

Color get_color(const Point<int> &t_point, const Point<double> &t_center, const int t_width, const int t_height, const double t_scale, const int max_iteration)
{
  const auto xscaled = t_point.x/(t_width/t_scale) + (t_center.x - (t_scale/2.0 ));
  const auto yscaled = t_point.y/(t_height/t_scale) + (t_center.y - (t_scale/2.0 ));

  auto x = xscaled;
  auto y = yscaled;

  int iteration = 0;

  auto stop_iteration = max_iteration;

  while ( iteration < stop_iteration )
  {
    if (((x*x) + (y*y)) > (2.0*2.0) && stop_iteration == max_iteration)
    {
      stop_iteration = iteration + 5;
    }

    auto xtemp = (x*x) - (y*y) + xscaled;
    y = (2.0*x*y) + yscaled;
    x = xtemp;
    ++iteration;
  }

  if (iteration == max_iteration)
  {
    return Color{0.0,0.0,0.0};
  } else {
    const auto value = ((iteration + 1) - (std::log(std::log(std::fabs(x * y))))/std::log(2.0));
    auto red = 0.0;
    auto green = 0.0;
    auto blue = 0.0;

    const auto colorval = std::floor(value * 10.0);

    if (colorval < 256)
    {
      red = colorval/256.0;
    } else {
      const auto colorband = std::floor( (static_cast<int>(colorval - 256) % 1024) / 256);
      const auto mod256 = static_cast<int>(colorval) % 256;
      if (colorband == 0)
      {
        red = 1.0;
        green = mod256 / 255.0;
        blue = 0.0;
      } else if (colorband == 1) {
        red = 1.0;
        green = 1.0;
        blue = mod256 / 255.0;
      } else if (colorband == 2) {
        red = 1.0;
        green = 1.0;
        blue = 1.0 - (mod256/255.0);
      } else {
        red = 1.0;
        green = 1.0 - (mod256/255.0);
        blue = 0.0;
      }
    }
    return Color{red, green, blue};
  }
}

void set_pixel(const Point<int> &t_point, const Color &t_color)
{
  *reinterpret_cast<uint8_t*>(0xA0000 + t_point.x + (t_point.y * 320)) = static_cast<uint8_t>(std::floor((t_color.r + t_color.g + t_color.b) / 3 * 255));
}

void region_calc(const Point<int> &t_tl, const Point<int> &t_br, const int t_width, const int t_height,
    const Point<double> &t_center, const double t_scale, const int t_max_iteration)
{
  for (auto y = t_tl.y; y < t_br.y; ++y)
  {
    for (auto x = t_tl.x; x < t_br.x; ++x)
    {
      const auto p = Point<int>{x,y};
      set_pixel(p, get_color(p, t_center, t_width, t_height, t_scale, t_max_iteration));
    }
  }
}

void Service::start()
{
  hw::KBM::init();
  VGA_gfx::set_mode(VGA_gfx::MODE_320_200_256);
  VGA_gfx::apply_default_palette();

  // full screen with blue color (1)
  memset(VGA_gfx::address(), 1, VGA_gfx::size());

  auto center = Point<double>{ 0.001643721971153, -0.822467633298876 };
  auto scale = 0.0000000001;

  region_calc(Point<int>{0,0}, Point<int>{320,200}, 320, 200, center, scale, 2000);
}
