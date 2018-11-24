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

#include <os.hpp>
#include <util/alloc_lstack.hpp>

#ifndef OS_MACHINE_MEMORY_HPP
#define OS_MACHINE_MEMORY_HPP

namespace os {
  using Machine_allocator =
    util::alloc::Lstack<util::alloc::Lstack_opt::merge, os::Arch::word_size>;

  /**  Extensible Memory class  **/
  class Machine::Memory : public Machine_allocator {
  public:
    using Alloc = Machine_allocator;
    using Alloc::Alloc;
    // TODO: Keep track of donated pools;
  };


  /**
   * C++17 std::allocator interface
   **/
  template <typename T>
  struct Machine::Allocator {
    using value_type = T;

    Allocator()
      : resource{os::machine().memory()}
    {}

    Allocator(os::Machine::Memory& mem)
      : resource{mem}
    {}

    template <class U>
    Allocator(const Allocator<U>& other) noexcept
      : resource{other.resource}
      { }

    T* allocate(std::size_t size) {
      auto res = reinterpret_cast<T*>(resource.allocate(size * sizeof(T)));
      if (res == nullptr)
        throw std::bad_alloc();
      return res;
    }

    void deallocate(T* ptr, std::size_t size) noexcept {
      resource.deallocate(ptr, size * sizeof(T));
    }

    bool operator==(const Allocator& other) const noexcept {
      return &resource == &(other.resource);
    }

    bool operator!=(const Allocator& other) const noexcept {
      return not (other == *this);
    }

    template< class U, class... Args >
    std::unique_ptr<U> make_unique( Args&&... args ) {
      void* addr = allocate(sizeof(U));
      auto deleter = [this](auto* ptr) { deallocate(ptr, sizeof(U)); };
      return std::unique_ptr<U>(new (addr) U(std::forward<Args>(args)...), deleter);
    };

    Machine::Memory& resource;
  };

}

#endif
