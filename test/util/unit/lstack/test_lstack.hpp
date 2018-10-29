// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2018 IncludeOS AS, Oslo, Norway
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

#ifndef LSTACK_COMMON_HPP
#define LSTACK_COMMON_HPP

//#define DEBUG_UNIT
//#undef NO_INFO

#include <common.cxx>

#include <algorithm>
#include <util/alloc_lstack.hpp>
#include <util/units.hpp>
#include <util/bitops.hpp>
#include <malloc.h>

#define QUOTE(X) #X
#define STR(X) QUOTE(X)

using namespace util;

template<typename L>
inline void print_summary(L& lstack)
{
  #ifndef DEBUG_UNIT
  return;
  #else

  FILLINE('=');
  printf("Summary: is_sorted: %s ", lstack.is_sorted ? "YES" : "NO" );
  printf("Pool: %#zx -> %#zx\n", lstack.pool_begin(), lstack.pool_end());
  printf("Alloc begin: %p Allocation end: %#zx\n", lstack.allocation_begin(), lstack.allocation_end());
  FILLINE('-');
  using Chunk = typename L::Node;
  const Chunk* ptr = (Chunk*)lstack.allocation_begin();
  if (ptr == nullptr) {
    printf("[0]\n");
    return;
  }

  while (ptr != nullptr) {
    printf("[%p->%p ", ptr, (std::byte*)ptr + ptr->size);
    for (int i = 0; i < ptr->size / 4096; i++)
    {
        printf("#");
    }

    printf("(%s)", util::Byte_r(ptr->size).to_string().c_str());

    if (ptr->next == nullptr) {
      printf("]->0");
      break;
    } else {
      printf("]->");
    }
    ptr = ptr->next;
  }
  printf("\n");
  FILLINE('=');
  #endif
}

namespace test {
  template <typename Alloc, auto Sz>
  struct pool {
    static constexpr size_t size = Sz;
    pool() : data{memalign(Alloc::align, size)}
    {
      stack.donate(data, size);
    }

    ~pool(){
      free(data);
    }

    uintptr_t begin() {
      return (uintptr_t)data;
    }

    uintptr_t end() {
      return (uintptr_t)data + size;
    }

    Alloc stack;
    void* data;
  };
}

inline std::ostream& operator<<(std::ostream& out, const util::alloc::Allocation& a) {
  out << std::hex << "[" << a.ptr << "," << (uintptr_t)((char*)a.ptr + a.size) << " ] ("
      << util::Byte_r(a.size) << ")" << std::dec;
  return out;
};

template <size_t Sz>
inline std::ostream& operator<<(std::ostream& out, const util::alloc::Node<Sz>& a) {
  out << std::hex << "[" << a.ptr << "," << (uintptr_t)((char*)a.ptr + a.size) << " ] ("
      << util::Byte_r(a.size) << ")" << std::dec;
  return out;
};


#endif
