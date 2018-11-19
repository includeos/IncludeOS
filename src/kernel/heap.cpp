// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2016 Oslo and Akershus University College of Applied Sciences
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

#include <kernel/os.hpp>
#include <util/bitops.hpp>
#include <util/units.hpp>
#include <kernel.hpp>

using namespace util::literals;

size_t brk_bytes_used();
size_t mmap_bytes_used();
size_t mmap_allocation_end();

static constexpr size_t default_max_mem = 2_GiB;

uintptr_t OS::heap_begin_ = 0;
uintptr_t OS::heap_max_   = default_max_mem;
uintptr_t OS::memory_end_ = default_max_mem;


size_t OS::heap_usage() noexcept
{
  return brk_bytes_used() + mmap_bytes_used();
}

size_t OS::heap_avail() noexcept
{
  return (heap_max() - heap_begin()) - heap_usage();
}

void OS::heap_trim() noexcept
{
  //malloc_trim(0);
}

uintptr_t OS::heap_max() noexcept
{
  return OS::heap_max_;
}

uintptr_t OS::heap_begin() noexcept
{
  return heap_begin_;
}
uintptr_t OS::heap_end() noexcept
{
  return mmap_allocation_end();
}

size_t OS::total_memuse() noexcept {
  return heap_usage() + OS::liveupdate_phys_size(OS::heap_max()) + heap_begin_;
}

constexpr size_t heap_alignment = 4096;
__attribute__((weak)) ssize_t __brk_max = 0x100000;

static bool __heap_ready = false;

extern void init_mmap(uintptr_t mmap_begin);


uintptr_t __init_brk(uintptr_t begin, size_t size);
uintptr_t __init_mmap(uintptr_t begin, size_t size);

namespace kernel {
  bool heap_ready() { return __heap_ready; }
}

namespace os {
  bool heap_ready() { return kernel::heap_ready(); }
}

void OS::init_heap(uintptr_t free_mem_begin, uintptr_t memory_end) noexcept {
  // NOTE: Initialize the heap before exceptions
  // cache-align heap, because its not aligned
  memory_end_ = memory_end;
  heap_max_   = memory_end-1;
  heap_begin_ = util::bits::roundto<heap_alignment>(free_mem_begin);
  auto brk_end  = __init_brk(heap_begin_, __brk_max);

  __init_mmap(brk_end, memory_end);
  __heap_ready = true;
}
