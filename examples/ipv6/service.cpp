#include <os>
#include <iostream>
#include <x86intrin.h>

//volatile int xxx = 333;

#define SSE_ALIGNED  __attribute__((aligned(16)))

void Service::start()
{
  volatile __m128i test1 SSE_ALIGNED;
  test1 = _mm_set1_epi32(333);
  
  volatile __m128i test2 SSE_ALIGNED;
  test2 = _mm_set1_epi32(111);
  
  volatile __m128i test SSE_ALIGNED;
  test = _mm_add_epi32(test1, test2);  
  
  volatile int* ints = (int*) &test;
  std::cout << ints[0] << std::endl;
}
