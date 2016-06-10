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
#ifndef HW_APIC_REVENANT_HPP
#define HW_APIC_REVENANT_HPP

#define BSP_LAPIC_IPI_IRQ  126
#define REV_STACK_SIZE     8192

#include <cstdint>
#include <hw/apic.hpp>
#include <deque>

extern "C"
void revenant_main(int, uintptr_t);

// Intel 3a  8.10.6.7: 128-byte boundary
typedef volatile int spinlock_t __attribute__((aligned(128)));

extern "C"
inline void lock(spinlock_t& lock) {
  while (__sync_lock_test_and_set(&lock, 1)) {
    while (lock) asm volatile("pause");
  }
}
extern "C"
inline void unlock(spinlock_t& lock) {
  __sync_synchronize(); // barrier
  lock = 0;
}

struct minimal_barrier_t
{
  void inc()
  {
    __sync_fetch_and_add(&val, 1);
  }
  
  void spin_wait(int max)
  {
    asm("mfence");
    while (this->val < max) {
      asm volatile("pause; nop;");
    }
  }
  
  void reset(int val)
  {
    asm("mfence");
    this->val = val;
  }
  
private:
  volatile int val = 0;
};

struct smp_stuff
{
  struct task {
    task(hw::APIC::smp_task_func a,
         hw::APIC::smp_done_func b)
      : func(a), done(b) {}
    
    hw::APIC::smp_task_func func;
    hw::APIC::smp_done_func done;
  };
  
  spinlock_t glock;
  minimal_barrier_t boot_barrier;
  
  hw::APIC::smp_done_func done_func;
  
  spinlock_t tlock;
  std::deque<task> tasks;
  
  spinlock_t flock;
  std::deque<hw::APIC::smp_done_func> completed;
};
extern smp_stuff smp;

#endif
