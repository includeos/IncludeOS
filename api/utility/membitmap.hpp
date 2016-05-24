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

#pragma once
#include <stdint.h>
#include <memstream>

/**
 * In-memory bitmap implementation
 * 
 * 
 **/

class MemBitmap
{
public:
  typedef uint32_t word;
  static const word WORD_MAX = UINT32_MAX;
  typedef int32_t  index_t;
  static const int CHUNK_SIZE = sizeof(word) * 8;
  
  MemBitmap() = default;
  MemBitmap(void* location, index_t chunks)
    : _chunks(chunks)
  {
    _data = reinterpret_cast<word*>(location);
  }
  
  // returns the boolean value of the bit located at @n
  bool operator[] (index_t n) const
  {
    return get(n);
  }
  bool get(index_t b) const
  {
    return _data[windex(b)] & (1 << woffset(b));
  }
  
  // return the bit-index of the first clear bit
  index_t first_free() const
  {
    // each word
    for (int i = 0; i < _chunks; i++)
    if (_data[i] != WORD_MAX) {
      // each bit
      for (int b = 0; b < CHUNK_SIZE; b++)
      if (!(_data[i] & (1 << b))) {
        return i * CHUNK_SIZE + b;
      }
    } // chunk
    return -1;
  }
  
  index_t first_set() const
  {
    for (int i = 0; i < _chunks; i++)
    if (_data[i])
    {
      int b = __builtin_ffs(_data[i]) - 1;
      return i * CHUNK_SIZE + b;
    }
    return -1;
  }
  
  void zero_all()
  {
    streamset32(_data, 0, _chunks);
  }
  void set(index_t b)
  {
    _data[windex(b)] |= 1 << woffset(b); 
  }
  void reset(index_t b)
  {
    _data[windex(b)] &= ~(1 << woffset(b));
  }
  void flip(index_t b)
  {
    _data[windex(b)] ^= 1 << woffset(b); 
  }
  
  void set_location(const void* location, index_t chunks)
  {
    this->_data = (word*) location;
    this->_chunks = chunks;
  }
  char* data() const noexcept
  {
    return (char*) _data;
  }
  index_t size() const noexcept
  {
    return _chunks * CHUNK_SIZE;
  }
  
  // bit-and two bitmaps together
  MemBitmap& operator&= (const MemBitmap& bmp)
  {
    for (index_t i = 0; i < _chunks; i++)
      _data[i] &= bmp._data[i];
    return *this;
  }
  void set_from_and(const MemBitmap& a, MemBitmap& b)
  {
    for (index_t i = 0; i < _chunks; i++)
      _data[i] = a._data[i] & b._data[i];
  }
  
private:
  index_t windex (index_t b) const { return b / CHUNK_SIZE; }
  index_t woffset(index_t b) const { return b & (CHUNK_SIZE-1); }
  
  word*   _data {nullptr};
  index_t _chunks;
};
