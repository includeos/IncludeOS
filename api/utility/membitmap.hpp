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
#include <cstdint>
#include <cstring>
#include <cassert>

/**
 * In-memory bitmap implementation
 * 
 * Not assigning memory area causes undefined behavior
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
    _data = (word*) location;
  }
  
  // returns the boolean value of the bit located at @n
  bool operator[] (index_t n) const noexcept
  {
    return get(n);
  }
  bool get(index_t b) const noexcept
  {
    return _data[windex(b)] & bit(b);
  }
  
  // return the bit-index of the first clear bit
  index_t first_free() const noexcept
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
  
  index_t first_set() const noexcept
  {
    for (int i = 0; i < _chunks; i++)
    if (_data[i])
    {
      int b = __builtin_ffs(_data[i]) - 1;
      return i * CHUNK_SIZE + b;
    }
    return -1;
  }
  index_t last_set() const noexcept
  {
    for (int i = _chunks-1; i >= 0; i--)
    if (_data[i])
    {
      int b = 31 - __builtin_clz(_data[i]);
      return i * CHUNK_SIZE + b;
    }
    return -1;
  }
  
  void zero_all() noexcept
  {
    memset(_data, 0, sizeof(word) * _chunks);
  }
  void set_all() noexcept
  {
    memset(_data, 0xFF, sizeof(word) * _chunks);
  }
  
  void set(index_t b) noexcept
  {
    _data[windex(b)] |= bit(b); 
  }
  void reset(index_t b) noexcept
  {
    _data[windex(b)] &= ~bit(b);
  }
  void flip(index_t b) noexcept
  {
    _data[windex(b)] ^= bit(b); 
  }
  
  void atomic_set(index_t b) noexcept
  {
    __sync_fetch_and_or(&_data[windex(b)], bit(b));
  }
  void atomic_reset(index_t b) noexcept
  {
    __sync_fetch_and_and(&_data[windex(b)], ~bit(b));
  }
  
  void set_location(const void* location, index_t chunks)
  {
    assert(location != nullptr && chunks != 0);
    this->_data = (word*) location;
    this->_chunks = chunks;
  }
  
  // returns data in bytes
  char* data() const noexcept
  {
    return (char*) _data;
  }
  // returns size in bytes of data section
  index_t size() const noexcept
  {
    return _chunks * sizeof(word);
  }
  
  // bit-and two bitmaps together
  // safe version
  MemBitmap& operator&= (const MemBitmap& bmp)
  {
    assert(bmp._chunks == _chunks);
    for (index_t i = 0; i < _chunks; i++)
      _data[i] &= bmp._data[i];
    return *this;
  }
  // unsafe version: must have data sections, and the number of chunks
  // must match (as there is no check!)
  void set_from_and(const MemBitmap& a, MemBitmap& b) noexcept
  {
    for (index_t i = 0; i < _chunks; i++)
        _data[i] = a._data[i] & b._data[i];
  }
  
  // might wanna look at individual chunks for
  // debugging purposes
  word get_chunk(index_t chunk) const
  {
    assert(chunk >= 0 && chunk < _chunks);
    return _data[chunk];
  }
  
private:
  index_t windex (index_t b) const { return b / CHUNK_SIZE; }
  index_t woffset(index_t b) const { return b & (CHUNK_SIZE-1); }
  index_t bit(index_t b) const { return 1 << woffset(b); }
  
  word*   _data {nullptr};
  index_t _chunks;
};
