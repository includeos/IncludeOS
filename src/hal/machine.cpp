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

#include <hal/machine.hpp>
#include <util/units.hpp>
#include <kernel.hpp>
#include <os>

#ifndef INFO_MACHINE
#ifndef USERSPACE_KERNEL
#define MINFO(fmt, ...) kprintf("[ Machine ] " fmt, ##__VA_ARGS__)
#else
#define MINFO(fmt, ...) printf("[ Machine ] " fmt, ##__VA_ARGS__)
#endif
#endif

using namespace util;

// Reserve some machine memory for e.g. devices
// (can still be used by heap as fallback).
static constexpr size_t reserve_mem = 1_MiB;

// Max percent of memory reserved by machine
static constexpr int  reserve_pct_max = 10;
static_assert(reserve_pct_max > 0 and reserve_pct_max < 90);

namespace os {

  Machine::Memory& Machine::memory() noexcept {
    return impl->memory();
  }

  void Machine::init() noexcept {
    impl->init();
  }

  const char* Machine::arch() noexcept {
    return impl->arch();
  }

  // Implementation details
  Machine* Machine::create(void* mem, size_t size) noexcept {
    char* mem_begin = (char*)mem + sizeof(Machine);
    return new(mem) Machine((void*)mem_begin, size - sizeof(Machine));
  }

  Machine::Machine(void* mem, size_t size) noexcept
  : impl {nullptr} {

    Expects(mem != nullptr);
    Expects(size > sizeof(detail::Machine) + Machine::Memory::min_alloc);

    // Placement new impl
    impl = {new (mem) detail::Machine{(char*)mem + sizeof(detail::Machine),
                                      size - sizeof(detail::Machine)}};
  }

}

// Detail implementations
namespace os::detail {

  Machine::Machine(void* raw_mem, size_t size)
    : mem_{
        (void*) bits::align(Memory::align, (uintptr_t)raw_mem),
        size - (bits::align(Memory::align, (uintptr_t)raw_mem) - (uintptr_t)raw_mem)
      },
      ptr_alloc_(mem_), parts_(ptr_alloc_), device_types_(mem_)
  {
#ifndef USERSPACE_KERNEL
    kprintf("[%s %s] constructor \n", arch(), name());
#endif
  }

  void Machine::init() {
    MINFO("Initializing heap\n");
    auto main_mem = memory().allocate_largest();
    MINFO("Main memory detected as %zu b\n", main_mem.size);

    const auto percent = main_mem.size / (100 / reserve_pct_max);
    const auto reserve = std::min(reserve_mem, percent);
    main_mem.size -= reserve;
    auto back = (uintptr_t)main_mem.ptr + main_mem.size - reserve;
    memory().deallocate((void*)back, reserve_mem);
    MINFO("Reserving %zu b for machine use \n", reserve);

#ifndef PLATFORM_x86_solo5
    const auto liu_steal = main_mem.size / (100 / 25);
    main_mem.size -= liu_steal;
    main_mem.size &= ~(uintptr_t) 0xFFF;
#endif
    kernel::init_heap((uintptr_t)main_mem.ptr, main_mem.size);
  }

  const char* Machine::arch() { return Arch::name; }
}
