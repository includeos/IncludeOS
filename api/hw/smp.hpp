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

#pragma once
#ifndef HW_SMP_HPP
#define HW_SMP_HPP

#include <cstdint>
#include <delegate>
#include <vector>

namespace hw {

  class SMP {
  public:
    typedef delegate<void()>  smp_task_func;
    typedef delegate<void()>  smp_done_func;

    // add tasks that will not necessarily start immediately
    // use work_signal() to guarantee work starts
    static void add_task(smp_task_func, smp_done_func);
    // call this to signal that work is queued up
    static void work_signal();
    static std::vector<smp_done_func> get_completed();

    static void init();
  };

}

#endif
