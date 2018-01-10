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
extern uintptr_t heap_begin;
extern uintptr_t heap_end;

uintptr_t OS::heap_usage() noexcept
{
  return ::heap_end;
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
  return ::heap_begin;
}
uintptr_t OS::heap_end() noexcept
{
  return ::heap_end;
}

uintptr_t OS::resize_heap(size_t size)
{
  uintptr_t new_end = heap_begin() + size;
  if (not size or size < heap_usage() or new_end > memory_end())
    return heap_max() - heap_begin();

  memory_map().resize(heap_begin(), size);
  heap_max_ = heap_begin() + size;
  return size;
}
