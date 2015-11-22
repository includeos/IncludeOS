#pragma once
#include <stdint.h>
#include <memstream>

/**
 * In-memory bitmap implementation
 * 
 * 
**/

namespace fs
{
  class MemBitmap
  {
  public:
    typedef uint64_t word;
    static const word WORD_MAX = UINT64_MAX;
    typedef int32_t  index_t;
    static const int CHUNK_SIZE = sizeof(word) * 8;
    
    MemBitmap() {}
    MemBitmap(void* location, index_t chunks)
    {
      _data = (word*) location;
      _size = chunks;
    }
    
    /** @return the boolean value of the bit located at @n 
	@param n the location of the bit*/
    inline bool operator[] (index_t n) const
    {
      return get(n);
    }
    inline bool get(index_t b) const
    {
      return _data[windex(b)] & (1 << woffset(b));
    }
    /** @return the bit-index of the first clear bit */
    index_t first_free() const
    {
      // each word
      for (index_t i = 0; i < _size; i++)
      if (_data[i] != WORD_MAX)
      {
        // each bit
        for (index_t b = 0; b < CHUNK_SIZE; b++)
        if (!(_data[i] & (1 << b)))
        {
          return i * CHUNK_SIZE + b;
        }
      } // chunk
      return -1;
    } // first_free()
    
    void zero_all()
    {
      streamset32(_data, 0, size());
    }
    void set(index_t b)
    {
      _data[windex(b)] |= 1 << (woffset(b)); 
    }
    void clear(index_t b)
    {
      _data[windex(b)] &= ~(1 << (woffset(b)));
    }
    void flip(index_t b)
    {
      _data[windex(b)] ^= 1 << (woffset(b)); 
    }
    
    inline char* data() const
    {
      return (char*) _data;
    }
    inline size_t size() const
    {
      return _size * CHUNK_SIZE;
    }
    
  private:
    inline index_t windex (index_t b) const { return b / CHUNK_SIZE; }
    inline index_t woffset(index_t b) const { return b % CHUNK_SIZE; }
    
    word*   _data;
    index_t _size;
  };
  
}