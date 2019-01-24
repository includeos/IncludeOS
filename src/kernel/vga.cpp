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
#if defined(ARCH_x86) || defined(ARCH_x86_64)
  #include <x86intrin.h>
#endif
#include <memstream>

static inline uint16_t
make_vgaentry(const char c, const uint8_t color) noexcept {
  uint16_t c16     = c;
  uint16_t color16 = color;
  return c16 | color16 << 8;
}
const uint16_t TextmodeVGA::DEFAULT_ENTRY =
    make_vgaentry(32, make_color(COLOR_LIGHT_GREY, COLOR_BLACK));

TextmodeVGA::TextmodeVGA() noexcept:
row{0}, column{0}
{
  this->color  = make_color(COLOR_WHITE, COLOR_BLACK);
  this->buffer = reinterpret_cast<uint16_t*>(0xB8000);
  clear();
}

uint16_t TextmodeVGA::get(uint8_t x, uint8_t y)
{
  const size_t index = y * VGA_WIDTH + x;
  return this->buffer[index];
}

void TextmodeVGA::put(const char c, uint8_t color, uint8_t x, uint8_t y) noexcept {
  putent(make_vgaentry(c, color), x, y);
}

void TextmodeVGA::put(const char c, uint8_t x, uint8_t y) noexcept {
  putent(make_vgaentry(c, this->color), x, y);
}

void TextmodeVGA::set_cursor(uint8_t x, uint8_t y) noexcept {
  this->column = x;
  this->row = y;
}

inline void TextmodeVGA::putent(uint16_t entry, uint8_t x, uint8_t y) noexcept {
  const size_t index = y * VGA_WIDTH + x;
  this->buffer[index] = entry;
}

void TextmodeVGA::increment(const int step) noexcept {
  this->column += step;
  if (this->column >= VGA_WIDTH) {
    newline();
  }
}


void TextmodeVGA::newline() noexcept {

  // Reset back to left side
  this->column = 0;

  // And finally move everything up one line, if necessary
  if (++this->row == VGA_HEIGHT) {
    this->row--;

    unsigned total {VGA_WIDTH * (VGA_HEIGHT - 1)};
    //should use IF SSE2 instead ?
#if defined(ARCH_x86) || defined(ARCH_x86_64)
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
#else
  // Copy rows upwards
  for (size_t n {0}; n < total; n++) {
    buffer[n]=buffer[n+VGA_WIDTH];
  }

  // Clear out the last row
  for (size_t n {0}; n < VGA_WIDTH; n++) {
    buffer[total+n]=DEFAULT_ENTRY;
  }
#endif
  }
}

void TextmodeVGA::clear() noexcept {
  this->row    = 0;
  this->column = 0;

  streamset16(buffer, DEFAULT_ENTRY, VGA_WIDTH * VGA_HEIGHT * 2);
}

void TextmodeVGA::write(const char c) noexcept {
  static const char CARRIAGE_RETURN = '\r';
  static const char LINE_FEED = '\n';

  if (c == LINE_FEED) {
    newline();
  } else if (c == CARRIAGE_RETURN) {
    // skip
  } else {
    put(c, this->color, this->column, this->row);
    increment(1);
  }
}

void TextmodeVGA::write(const char* data, const size_t len) noexcept {
  for (size_t i = 0; i < len; ++i)
    write(data[i]);
}
