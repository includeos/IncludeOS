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

#include <kernel/vga.hpp>
#include <cstring>
#include <x86intrin.h>

static uint16_t make_vgaentry(const char c, const uint8_t color) noexcept {
  uint16_t c16     = c;
  uint16_t color16 = color;
  return c16 | color16 << 8;
}
const uint16_t ConsoleVGA::DEFAULT_ENTRY = 
			  make_vgaentry(32, make_color(COLOR_LIGHT_GREY, COLOR_BLACK));

ConsoleVGA::ConsoleVGA() noexcept:
row{0}, column{0}
{
  this->color  = make_color(COLOR_WHITE, COLOR_BLACK);
  this->buffer = reinterpret_cast<uint16_t*>(0xB8000);
  clear();
}

void ConsoleVGA::setColor(const uint8_t color) noexcept {
  this->color = color;
}

void ConsoleVGA::putEntryAt(const char c, const uint8_t color, const size_t x, const size_t y) noexcept {
  put(make_vgaentry(c, color), x, y);
}

void ConsoleVGA::putEntryAt(const char c, const size_t x, const size_t y) noexcept {
  put(make_vgaentry(c, this->color), x, y);
}

void ConsoleVGA::setCursorAt(const size_t x, const size_t y) noexcept {
  this->column = x;
  this->row = y; 
}

inline void ConsoleVGA::put(uint16_t entry, size_t x, size_t y) noexcept {
  const size_t index = y * VGA_WIDTH + x;
  this->buffer[index] = entry;
}

void ConsoleVGA::increment(const int step) noexcept {
  this->column += step;
  if (this->column >= VGA_WIDTH) {
    newline();
  }
}


void ConsoleVGA::newline() noexcept {
  
  // Reset back to left side
  this->column = 0;

  // And finally move everything up one line, if necessary
  if (++this->row == VGA_HEIGHT) {
    this->row--;
    
    unsigned total {VGA_WIDTH * (VGA_HEIGHT - 1)};
    __m128i scan;
    
    // Copy rows upwards
    for (size_t n {0}; n < total; n += 8) {
      scan = _mm_load_si128(reinterpret_cast<__m128i*>(&buffer[n + VGA_WIDTH]));
      _mm_store_si128(reinterpret_cast<__m128i*>(&buffer[n]), scan);
    }
    
    // Clear out the last row
    scan = _mm_set1_epi16(DEFAULT_ENTRY);
    
    for (size_t n {0}; n < VGA_WIDTH; n += 8) {
      _mm_store_si128(reinterpret_cast<__m128i*>(&buffer[total + n]), scan);
    }
  }
}

void ConsoleVGA::clear() noexcept {
  this->row    = 0;
  this->column = 0;
  
  for (size_t x {0}; x < (VGA_WIDTH * VGA_HEIGHT); ++x)
    buffer[x] = DEFAULT_ENTRY;
}

void ConsoleVGA::write(const char c) noexcept {
  static const char NEWLINE {'\n'};
  
  if (c == NEWLINE) {
    newline();
  } else {
    putEntryAt(c, this->color, this->column, this->row);
    increment(1);
  }
}

void ConsoleVGA::write(const char* data, const size_t len) noexcept {
  for (size_t i {0}; i < len; ++i) write(data[i]);
}
