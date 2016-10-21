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

#ifndef UTIL_RINGBUFFER_HPP
#define UTIL_RINGBUFFER_HPP

#include <cstring>

class RingBuffer {
public:
  RingBuffer(int size)
    : cap(size+1), start(0), end(0)
  {
    assert(size > 1);
    this->buffer = new char[capacity()];
  }
  ~RingBuffer()
  {
    delete[] this->buffer;
  }

  int write(const void* buffer, int length) noexcept
  {
    const char* data = (const char*) buffer;
    const int fspace = free_space();
    if (length > fspace) {
      length = fspace;
      if (length == 0) return 0;
    }

    // check if we are going around the buffer ...
    int wrap = (end + length) - this->cap;
    
    if (wrap > 0) {
      memcpy(at_end() ,    data, wrap);
      memcpy(this->buffer, data + wrap, length - wrap);
    }
    else {
      memcpy(at_end(), data, length);
    }
    // make sure it wraps properly around
    this->end = (this->end + length) % capacity();
    return length;
  }

  int read(char* dest, int length) noexcept
  {
    const int s = size();
    if (length > s) {
      length = s;
      if (length == 0) return 0;
    }

    // check if we are going around the buffer ...
    int wrap = (start + length) - this->cap;
    
    if (wrap > 0) {
      memcpy(dest,        at_start(),   wrap);
      memcpy(dest + wrap, this->buffer, length - wrap);
    } else {
      memcpy(dest, at_start(), length);
    }
    // make sure it wraps properly around
    this->start = (this->start + length) % capacity();
    return length;
  }

  int discard(int length) noexcept
  {
    // do nothing if not enough used space to discard anyway
    if (length > size()) return 0;
    // commit no-op write and return length
    this->start = (this->start + length) % capacity();
    return length;
  }

  int capacity() const noexcept {
    return this->cap;
  }
  int size() const noexcept {
    if (end >= start)
        return end - start;
    else
        return (cap - start) + end;
  }
  int free_space() const noexcept {
    return capacity() - size();
  }

  bool full() const noexcept {
    return size() == capacity();
  }
  bool empty() const noexcept {
    return size() == 0;
  }

private:
  const char* at_start() const noexcept {
    return &this->buffer[this->start];
  }
  char* at_end() const noexcept {
    return &this->buffer[this->end];
  }

  char* buffer;
  int   cap;
  int  start;
  int  end;
};

#endif
