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

#include <kernel/rdrand.hpp>
#include <kernel/cpuid.hpp>
#define __MM_MALLOC_H
#if defined(ARCH_x86) || defined(ARCH_x86_64)
  #include <immintrin.h>
__attribute__((target("rdrnd")))
bool rdrand16(uint16_t* result)
{
  int res = 0;
  while (res == 0)
    {
      res = _rdrand16_step(result);
    }
  return (res == 1);
}

__attribute__((target("rdrnd")))
bool rdrand32(uint32_t* result)
{
  int res = 0;
  while (res == 0)
    {
      res = _rdrand32_step(result);
    }
  return (res == 1);
}
#else
#warning NO_PROPER_RDRAND_16_DEFINED
#warning NO_PROPER_RDRAND_32_DEFINED
//__attribute__((target("rdrnd")))
bool rdrand16(uint16_t* result)
{
  *result=(*result^1)**result;
  return true;
}

//__attribute__((target("rdrnd")))
bool rdrand32(uint32_t* result)
{
  *result=(*result^1)**result;
  return true;
}
#endif
