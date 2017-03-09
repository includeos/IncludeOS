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

extern "C"
void revenant_main(int);

struct smp_stuff
{
  struct task {
    task(SMP::task_func a,
         SMP::done_func b)
      : func(a), done(b) {}
    
    SMP::task_func func;
    SMP::done_func done;
  };
  
  minimal_barrier_t boot_barrier;
  
  spinlock_t tlock;
  std::deque<task> tasks;
  
  spinlock_t flock;
  std::deque<SMP::done_func> completed;
};
extern smp_stuff smp;

#endif
