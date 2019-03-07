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

#include <hal/machine.hpp>
#include <util/units.hpp>
#include <kernel/memory.hpp>
#include <string>

// Demangle
extern "C" char* __cxa_demangle(const char* mangled_name,
                            char*       output_buffer,
                            size_t*     length,
                            int*        status);

namespace os {
  using Machine_str = std::basic_string<char,
                                        std::char_traits<char>,
                                        os::Machine::Allocator<char>>;

  inline Machine_str demangle(const char* name) {
    using namespace util::literals;

    if (not mem::heap_ready() or name == nullptr) {
      return name;
    }

    int status = -1;
    size_t size = 1_KiB;

    auto str = Machine_str{};
    str.reserve(size);
    char* buf = str.data();
    buf =  __cxa_demangle(name, buf, &size, &status);

    if (UNLIKELY(status != 0)) {
      return {name};
    }

    return str;
  }
}
