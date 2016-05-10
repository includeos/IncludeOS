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

#define REV_STACK_SIZE   8192

#include <cstdint>

extern "C"
void revenant_main(int, uintptr_t);

typedef volatile int spinlock_t;

extern "C"
inline void lock(spinlock_t& lock) {
  while (__sync_lock_test_and_set(&lock, 1)) {
    while (lock);
  }
}
extern "C"
inline void unlock(spinlock_t& lock) {
  __sync_synchronize(); // barrier
  lock = 0;
}

#endif
