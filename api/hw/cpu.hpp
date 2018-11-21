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

#ifndef OS_HW_CPU
#define OS_HW_CPU

#include <util/units.hpp>

namespace os {
  util::KHz cpu_freq();
}

namespace os::experimental::hw {
  struct CPU {
    struct Task;
    auto frequency();
    void add_task(Task t);
    void signal();
    std::vector<std::reference_wrapper<Task>> tasks();
  };
}

#endif
