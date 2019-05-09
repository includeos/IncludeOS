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

#ifndef KERNEL_VGA_HPP
#define KERNEL_VGA_HPP

#include <os>
#include <cstdint>

class TextmodeVGA {
public:
  enum vga_color {
    COLOR_BLACK         = 0,
    COLOR_BLUE          = 1,
    COLOR_GREEN         = 2,
    COLOR_CYAN          = 3,
    COLOR_RED           = 4,
    COLOR_MAGENTA       = 5,
    COLOR_BROWN         = 6,
    COLOR_LIGHT_GREY    = 7,
    COLOR_DARK_GREY     = 8,
    COLOR_LIGHT_BLUE    = 9,
    COLOR_LIGHT_GREEN   = 10,
    COLOR_LIGHT_CYAN    = 11,
    COLOR_LIGHT_RED     = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_LIGHT_BROWN   = 14,
    COLOR_WHITE         = 15,
  };

  static const int VGA_WIDTH  {80};
  static const int VGA_HEIGHT {25};

  os::print_func get_print_handler() {
    return {this, &TextmodeVGA::write};
  }

  constexpr static uint8_t make_color(const vga_color fg, const vga_color bg) noexcept
  { return fg | bg << 4; }

  void write(const char* data, const size_t len) noexcept;
  void clear() noexcept;

  uint16_t get(uint8_t x, uint8_t y);
  void put(const char, uint8_t color, uint8_t x, uint8_t y) noexcept;
  void put(const char, uint8_t x, uint8_t y) noexcept;
  void newline() noexcept;

  void set_cursor(uint8_t x, uint8_t y) noexcept;
  size_t get_x();
  size_t get_y();

  void set_color(vga_color c);

  static TextmodeVGA& get() {
    static TextmodeVGA vga;
    return vga;
  }

private:
  explicit TextmodeVGA() noexcept;

  void increment(int) noexcept;
  void write(char) noexcept;
  static const uint16_t DEFAULT_ENTRY;
  void putent(uint16_t, uint8_t, uint8_t) noexcept;

  size_t    row;
  size_t    column;
  uint8_t   color;
  uint16_t* buffer;
};

inline size_t TextmodeVGA::get_x() {
  return this->column;
}
inline size_t TextmodeVGA::get_y() {
  return this->row;
}
inline void TextmodeVGA::set_color(vga_color c)
{
  this->color = c;
}

#endif //< KERNEL_VGA_HPP
