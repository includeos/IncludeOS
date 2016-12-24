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
#ifndef NET_TCP_READ_BUFFER_HPP
#define NET_TCP_READ_BUFFER_HPP

#include <cassert>

#include "common.hpp" // buffer_t

namespace net {
namespace tcp {

/*
  Wrapper around a buffer that receives data.
*/
struct ReadBuffer {
  buffer_t buffer;
  size_t remaining;
  size_t offset;

  ReadBuffer(buffer_t buf, size_t length, size_t offs = 0)
    : buffer(buf), remaining(length-offs), offset(offs) {}

  inline size_t capacity() const
  { return remaining + offset; }

  inline bool empty() const
  { return offset == 0; }

  inline bool full() const
  { return remaining == 0; }

  inline size_t size() const
  { return offset; }

  inline uint8_t* begin() const
  { return buffer.get(); }

  inline uint8_t* pos() const
  { return buffer.get() + offset; }

  inline uint8_t* end() const
  { return buffer.get() + capacity(); }

  size_t advance(size_t length) {
    assert(length <= remaining);
    offset += length;
    remaining -= length;
    return length;
  }

  void clear() {
    remaining = capacity();
    offset = 0;
    buffer = nullptr;
  }

  /*
    Renews the ReadBuffer by assigning a new buffer_t, releasing ownership
  */
  void renew() {
    clear();
    buffer = new_shared_buffer(remaining);
  }
}; // < ReadBuffer

} // < namespace tcp
} // < namespace net

#endif // < NET_TCP_READ_BUFFER_HPP
