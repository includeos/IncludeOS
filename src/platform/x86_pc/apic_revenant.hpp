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
#ifndef X86_APIC_REVENANT_HPP
#define X86_APIC_REVENANT_HPP

#include "smp.hpp"
#include <cstdint>
#include <deque>
#include <membitmap>
#include <vector>

extern "C" void revenant_main(int);

namespace x86 {
struct smp_task {
  smp_task(SMP::task_func a,
           SMP::done_func b)
   : func(a), done(b) {}

  SMP::task_func func;
  SMP::done_func done;
};

struct smp_stuff
{
  uintptr_t stack_base;
  uintptr_t stack_size;
  minimal_barrier_t boot_barrier;
  uint32_t  bmp_storage[1] = {0};
  std::vector<int> initialized_cpus {0};
  MemBitmap bitmap{&bmp_storage[0], 1};
};


extern smp_stuff smp_main;

struct smp_system_stuff
{
  spinlock_t tlock = 0;
  spinlock_t flock = 0;
  std::vector<smp_task> tasks;
  std::vector<SMP::done_func> completed;
  bool work_done;
};
 extern SMP::Array<smp_system_stuff> smp_system;
}

#endif
