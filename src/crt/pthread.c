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

#include <stdio.h>
#define UNLOCKED 0
#define LOCKED   1

extern void panic(const char* why);
typedef int spinlock_t;

void yield()
{
  printf("yield() called, but not implemented yet!");
}

static int compare_and_swap(volatile spinlock_t* addr)
{
  const int expected = UNLOCKED;
  const int newval   = LOCKED;
  return __sync_val_compare_and_swap(addr, expected, newval);
}


int pthread_mutex_init(volatile spinlock_t* lock)
{
  *lock = UNLOCKED;
  return 0;
}
int pthread_mutex_destroy(volatile spinlock_t* lock)
{
  (void) lock;
  return 0;
}

int pthread_mutex_lock(volatile spinlock_t* lock)
{
  while (!compare_and_swap(lock))
      yield();
  return 0;
}

int pthread_mutex_unlock(volatile spinlock_t* lock)
{
  *lock = UNLOCKED;
  return 0;
}
