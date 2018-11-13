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

#ifndef OS_MACHINE_HPP
#define OS_MACHINE_HPP

#include <vector>
#include <memory>

#include <arch.hpp>
#include <util/allocator.hpp>

namespace os {
  namespace detail { class Machine; }

  /**
   * Hardware abstracttion layer (HAL) entry point.
   * Provides raw memory and storage for all abstract devices etc.
   * The Machine class is partially implemented in the detail namespace. The
   * remainder is implemented directly by the various platforms.
   **/
  class Machine {
  public:
    class  Memory;
    //using Memory = util::alloc::Lstack<util::alloc::Lstack_opt::merge, os::Arch::word_size>;

    /** Get raw physical memory **/
    Memory& memory() noexcept;

    template <typename T>
    using Allocator = mem::Allocator<T, Memory>;

    /** Get a std::allocator conforming version of raw Memory **/
    template <typename T>
    Allocator<T>& allocator();

    const char* name() noexcept;
    const char* id()   noexcept;
    const char* arch() noexcept;

    void init() noexcept;
    void poweroff() noexcept;
    void reboot() noexcept;

    //
    // Machine parts - devices and anything else
    //

    template <typename T>
    using Ref = std::reference_wrapper<T>;

    template<typename T>
    using Vector = std::vector<Ref<T>, Allocator<Ref<T>>>;

    template <typename T>
    ssize_t add(std::unique_ptr<T> part) noexcept;

    template <typename T, typename... Args>
    ssize_t add_new(Args&&... args);

    template <typename T>
    void remove(int i);

    template <typename T>
    Vector<T> get();

    template <typename T>
    T& get(int i);

    Machine(void* mem_begin, size_t memsize) noexcept;

    /** Factory function for creating machine instance in-place **/
    static Machine* create(uintptr_t mem, uintptr_t mem_end) noexcept;

  private:
    std::unique_ptr<detail::Machine> impl;
  };

  // Get the Machine instance for the current context.
  Machine& machine();

} // namespace os

#include "detail/machine.hpp"
#endif // OS_MACHINE_HPP
