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
