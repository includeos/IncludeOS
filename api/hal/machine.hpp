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

    /** Get raw physical memory **/
    Memory& memory() noexcept;

    template <typename T>
    struct Allocator;

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
    Vector<T> get();

    template <typename T>
    T& get(int i);

    template <typename T>
    void remove(int i);

    template <typename T>
    ssize_t count();

    inline void deactivate_devices();
    inline void print_devices() const;

    Machine(void* mem, size_t size) noexcept;

    /** Factory function for creating machine instance in-place **/
    static Machine* create(void* mem, size_t size) noexcept;

  private:
    detail::Machine* impl;
  };

} // namespace os

#include "detail/machine.hpp"
#endif // OS_MACHINE_HPP
