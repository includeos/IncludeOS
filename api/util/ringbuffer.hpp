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
#include <cassert>
#include <cstdio>

/**
 *  See:
 *  HeapRingBuffer
 *  MemoryRingBuffer
**/

class RingBuffer {
public:
  int write(const void* buffer, int length) noexcept
  {
    const char* data = (const char*) buffer;
    if (length > free_space()) {
      length = free_space();
      if (length == 0) return 0;
    }

    // check if we are going around the buffer ...
    int wrap = (end + length) - this->cap;

    if (wrap > 0) {
      memcpy(at_end() ,    data, length - wrap);
      memcpy(this->buffer, data + length - wrap, wrap);
    }
    else {
      memcpy(at_end(), data, length);
    }
    this->used += length;
    // make sure it wraps properly around
    this->end = (this->end + length) % capacity();
    return length;
  }

  int read(char* dest, int length) noexcept
  {
    if (length > used_space()) {
      length = used_space();
      if (length == 0) return 0;
    }

    // check if we are going around the buffer ...
    int wrap = (start + length) - this->cap;

    if (wrap > 0) {
      memcpy(dest,                 at_start(),   length - wrap);
      memcpy(dest + length - wrap, this->buffer, wrap);
    } else {
      memcpy(dest, at_start(), length);
    }
    this->used -= length;
    // make sure it wraps properly around
    this->start = (this->start + length) % capacity();
    return length;
  }

  int discard(int length) noexcept
  {
    // do nothing if not enough used space to discard anyway
    if (length > used_space()) return 0;
    // commit no-op write and return length
    this->used -= length;
    this->start = (this->start + length) % capacity();
    return length;
  }

  int capacity() const noexcept {
    return this->cap;
  }
  int size() const noexcept {
    return used_space();
  }

  int used_space() const noexcept {
    return this->used;
  }
  int free_space() const noexcept {
    return capacity() - used_space();
  }

  bool full() const noexcept {
    return used_space() == capacity();
  }
  bool empty() const noexcept {
    return used_space() == 0;
  }

  bool is_valid() const noexcept {
    return (used <= cap and cap > 0)
      and (start < cap and start >= 0)
      and (end < cap and end >= 0);
  }

  // rotate buffer until it becomes sequential
  const char* sequentialize()
  {
    // not optimized, but if we assume the buffer is always full,
    // then there is no point in doing anything here
    int count = 0;
    int offset = 0;

    while (count < size())
    {
      int index = offset;
      const char tmp  = buffer[index];
      int index2 = (start + index) % capacity();

      while (index2 != offset)
      {
        buffer[index] = buffer[index2];
        count++;

        index = index2;
        index2 = (start + index) % capacity();
      }

      buffer[index] = tmp;
      count++;

      offset++;
    }
    // update internals
    this->start = 0;
    this->end = size();
    return this->buffer;
  }

  const char* data() const noexcept {
    return this->buffer;
  }

protected:
  RingBuffer(void* rbuffer, int size)
    : cap(size), buffer((char*) rbuffer)
  {
    assert(size > 0);
  }

  const char* at_start() const noexcept {
    return &this->buffer[this->start];
  }
  char* at_end() const noexcept {
    return &this->buffer[this->end];
  }

  const int  cap;
  int  start = 0;
  int  end   = 0;
  int  used  = 0;
  char* buffer;
};

class HeapRingBuffer : public RingBuffer {
public:
  HeapRingBuffer(int size)
    : RingBuffer(new char[size], size) {}
  ~HeapRingBuffer() {
    delete[] this->buffer;
  }
};

class MemoryRingBuffer : public RingBuffer {
public:
  MemoryRingBuffer(void* location, int size)
    : RingBuffer(location, size) {}

  MemoryRingBuffer(void* loc, const int cap,
                   int start, int end, int used)
    : RingBuffer(loc, cap)
  {
    this->start = start;
    this->end   = end;
    this->used  = used;
  }

  ~MemoryRingBuffer() {}
};

template <int N>
class FixedRingBuffer : public RingBuffer {
public:
  FixedRingBuffer()
    : RingBuffer(m_buffer, N) {}
  ~FixedRingBuffer() {}

private:
  char m_buffer[N];
};

#endif
