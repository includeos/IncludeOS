// -*-C++-*-
// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 IncludeOS AS, Oslo, Norway
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

#include <arch.hpp>
#include <kernel/memory.hpp>

__attribute__((weak))
void __arch_init_paging()
{
  INFO("aarch64", "Paging not enabled by default on");
}

namespace os {
namespace mem {
  __attribute__((weak))
  Map map(Map m, const char* name) {
    return {};
  }

  template <>
  const size_t Mapping<os::mem::Access>::any_size = 4096;
}
}
