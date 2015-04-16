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
    typedef int32_t  index_t;
    static const int CHUNK_SIZE = sizeof(word) * 8;
    
    MemBitmap() {}
    MemBitmap(void* location, index_t chunks)
    {
      this->data = (word*) location;
      this->size = chunks;
    }
    
    // returns the boolean value of the bit located at @n
    bool operator[] (index_t n) const
    {
      return get(n);
    }
    bool get(index_t b) const
    {
      return data[windex(b)] & (1 << woffset(b));
    }
    // return the bit-index of the first clear bit
    index_t first_free() const
    {
      for (index_t i = 0; i < this->size; i++)
      {
        if (data[i])
        {
          for (index_t b = 0; b < CHUNK_SIZE; b++)
          {
            if (data[i] & (1 << b))
              return i * CHUNK_SIZE + b;
          } // bit
        }
      } // chunk
      return -1;
      
    } // first_free()
    
    void zero_all()
    {
      streamset32(data, 0, size_bytes());
    }
    void set(index_t b)
    {
      data[windex(b)] |= 1 << (woffset(b)); 
    }
    void clear(index_t b)
    {
      data[windex(b)] &= ~(1 << (woffset(b)));
    }
    void flip(index_t b)
    {
      data[windex(b)] ^= 1 << (woffset(b)); 
    }
    
    char* location() const
    {
      return (char*) data;
    }
    size_t size_bytes() const
    {
      return size * CHUNK_SIZE;
    }
    
  private:
    inline index_t windex(index_t b)  const { return b / CHUNK_SIZE; }
    inline index_t woffset(index_t b) const { return b % CHUNK_SIZE; }
    
    word*   data;
    index_t size;
  };
  
}