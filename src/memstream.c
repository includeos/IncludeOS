#include <utility/memstream.h>

#include <x86intrin.h>
#include <stdint.h>

void* stream_alloc(size_t n)
{
  // allocate aligned address + space for pointer
  void* mem = malloc(n + (SSE_SIZE-1) + sizeof(void*));
  if (!mem) return mem;
  
  // calculate placement for aligned memory
  intptr_t addr = ((intptr_t) mem + sizeof(void*) + (SSE_SIZE-1)) & ~(SSE_SIZE-1);
  void* ptr = (void*) addr;
  // remember the malloc() result
  ((void**)ptr)[-1] = mem;
  // return pointer to aligned memory
  return ptr;
}
void stream_free(void* ptr)
{
  // free all memory by freeing the malloc() result
  if (ptr) free(((void**) ptr)[-1]);
}

void* streamcpy(char* dest, const char* src, size_t n)
{
  while (n >= SSE_SIZE)
  {
    __m128i data = _mm_load_si128((__m128i*) src);
    _mm_stream_si128((__m128i*) dest, data);
    
    dest += SSE_SIZE;
    src  += SSE_SIZE;
    
    n -= SSE_SIZE;
  }
  while (n--)
  {
    *dest++ = *src++;
  }
  return dest;
}
void* streamucpy(char* dest, const char* usrc, size_t n)
{
  while (n >= SSE_SIZE)
  {
    __m128i data = _mm_loadu_si128((__m128i*) usrc);
    _mm_stream_si128((__m128i*) dest, data);
    
    dest += SSE_SIZE;
    usrc += SSE_SIZE;
    
    n -= SSE_SIZE;
  }
  while (n--)
  {
    *dest++ = *usrc++;
  }
  return dest;
}

void* streamset(char* dest, char value, size_t n)
{
  __m128i data = _mm_set1_epi8(value);
  while (n >= SSE_SIZE)
  {
    _mm_stream_si128((__m128i*) dest, data);
    
    dest += SSE_SIZE;
    n    -= SSE_SIZE;
  }
  while (n--)
  {
    *dest++ = value;
  }
  return dest;
}
