// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2017 Oslo and Akershus University College of Applied Sciences
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

#include <pthread.h>
#include <cstdio>

/*
#include <fiber>
#include <unordered_map>
std::unordered_map<pthread_t, Fiber> fibers;

int pthread_create(pthread_t* th, const pthread_attr_t* attr, void *(*func)(void *), void* args) {

  static int __thread_id__ = 0;
  auto new_id = __thread_id__++;

  fibers.emplace(new_id, Fiber{func, args});
  fibers[new_id].start();

  *th = new_id;
  return 0;
}

int pthread_join(pthread_t thread, void **value_ptr) {
  void* retval = fibers[thread].ret<void*>();
  *value_ptr = retval;
  return 0;
}
*/

#include <smp>
int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)
{
  if (mutex == nullptr) return 1;
  mutex->spinlock = 0;
}
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
  if (mutex == nullptr) return 1;
  lock(mutex->spinlock);
  SMP_PRINT("Locked %p\n", mutex);
  return 0;
}
int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
  if (mutex == nullptr) return 1;
  if (mutex->spinlock) return 1;
  lock(mutex->spinlock);
  return 0;
}
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  if (mutex == nullptr) return 1;
  SMP_PRINT("Unlocked %p\n", mutex);
  unlock(mutex->spinlock);
  return 0;
}
int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
  return 0;
}

int pthread_once(pthread_once_t*, void (*routine)())
{
  thread_local bool executed = false;
  if (!executed) {
    printf("Executing pthread_once routine %p\n", routine);
    executed = true;
    routine();
  }
  return 0;
}

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex)
{
  SMP_PRINT("pthread_cond_wait\n")
  return 0;
}
int pthread_cond_broadcast(pthread_cond_t* cond)
{
  SMP_PRINT("pthread_cond_broadcast\n")
  return 0;
}

#include <vector>
std::vector<const void*> key_vec;
spinlock_t             key_lock = 0;

void* pthread_getspecific(pthread_key_t key)
{
  scoped_spinlock spinlock(key_lock);
  return (void*) key_vec.at(key);
}
int pthread_setspecific(pthread_key_t key, const void *value)
{
  scoped_spinlock spinlock(key_lock);
  key_vec.at(key) = value;
  return 0;
}
int pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
{
  scoped_spinlock spinlock(key_lock);
  key_vec.push_back(nullptr);
  *key = key_vec.size()-1;
  return 0;
}
