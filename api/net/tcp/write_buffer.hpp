// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
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

#pragma once
#ifndef NET_TCP_WRITE_BUFFER_HPP
#define NET_TCP_WRITE_BUFFER_HPP

#include "common.hpp"
#include <common>

namespace net {
namespace tcp {
/*
  Wrapper around a buffer that contains data to be written.
*/
struct WriteBuffer {
  buffer_t buffer;
  size_t remaining;
  size_t offset;
  size_t acknowledged;
  bool push;

  WriteBuffer(buffer_t buf, size_t length, bool PSH, size_t offs = 0)
    : buffer(buf), remaining(length-offs), offset(offs), acknowledged(0), push(PSH) {}

  size_t length() const { return remaining + offset; }

  bool done() const { return acknowledged == length(); }

  uint8_t* begin() const { return buffer.get(); }

  uint8_t* pos() const { return buffer.get() + offset; }

  uint8_t* end() const { return buffer.get() + length(); }

  bool advance(size_t length) {
    Expects(length <= remaining);
    offset += length;
    remaining -= length;
    return length > 0;
  }

  size_t acknowledge(size_t bytes) {
    auto acked = std::min(bytes, length()-acknowledged);
    acknowledged += acked;
    return acked;
  }

  operator buffer_t() const noexcept
  { return buffer; }

  bool operator==(const WriteBuffer& wb) const
  { return buffer == wb.buffer && length() == wb.length(); }

}; // < struct WriteBuffer

} // < namespace net
} // < namespace tcp

#endif // < NET_TCP_WRITE_BUFFER_HPP
