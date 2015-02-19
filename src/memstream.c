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

void* streamcpy(void* dest, const void* srce, size_t n)
{
  char* dst       = (char*) dest;
  const char* src = (const char*) srce;
  
  // copy up to 15 bytes until SSE-aligned
  while (((intptr_t) dst & (SSE_SIZE-1)) && n)
  {
    *dst++ = *src++; n--;
  }
  // copy SSE-aligned
  while (n >= SSE_SIZE)
  {
    __m128i data = _mm_load_si128((__m128i*) src);
    _mm_stream_si128((__m128i*) dst, data);
    
    dst += SSE_SIZE;
    src += SSE_SIZE;
    
    n -= SSE_SIZE;
  }
  // copy remainder
  while (n--)
  {
    *dst++ = *src++;
  }
  return dst;
}
void* streamucpy(void* dest, const void* usrc, size_t n)
{
  char* dst       = (char*) dest;
  const char* src = (const char*) usrc;
  
  // copy up to 15 bytes until SSE-aligned
  while (((intptr_t) dst & (SSE_SIZE-1)) && n)
  {
    *dst++ = *src++; n--;
  }
  // copy SSE-aligned
  while (n >= SSE_SIZE)
  {
    __m128i data = _mm_loadu_si128((__m128i*) src);
    _mm_stream_si128((__m128i*) dst, data);
    
    dst  += SSE_SIZE;
    src += SSE_SIZE;
    
    n -= SSE_SIZE;
  }
  // copy remainder
  while (n--)
  {
    *dst++ = *src++;
  }
  return dst;
}

void* streamset(void* dest, char value, size_t n)
{
  char* dst = dest;
  
  // memset up to 15 bytes until SSE-aligned
  while (((intptr_t) dst & (SSE_SIZE-1)) && n)
  {
    *dst++ = value; n--;
  }
  // memset SSE-aligned
  __m128i data = _mm_set1_epi8(value);
  while (n >= SSE_SIZE)
  {
    _mm_stream_si128((__m128i*) dst, data);
    
    dst += SSE_SIZE;
    n   -= SSE_SIZE;
  }
  // memset remainder
  while (n--)
  {
    *dst++ = value;
  }
  return dst;
}
