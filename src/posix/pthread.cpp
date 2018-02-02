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
#include <smp>
#include <fiber>
#include <unordered_map>
#include <vector>
#include <time.h>
#include <posix_strace.hpp>

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
  *value_ptr = fibers[thread].ret<void*>();
  return 0;
}


int sched_yield()
{
  SMP_PRINT("WARNING: Sched yield called\n");
}

thread_local pthread_t tself = 0;
pthread_t pthread_self()
{
  return tself;
}

int pthread_detach(pthread_t th)
{
  printf("pthread_detach %d\n", th);
  return 0;
}
int pthread_equal(pthread_t t1, pthread_t t2)
{
  return t1 == t2;
}


int pthread_mutex_init(pthread_mutex_t* mutex, const pthread_mutexattr_t* attr)
{
  if (mutex == nullptr) return 1;
  *mutex = 0;
  return 0;
}
int pthread_mutex_lock(pthread_mutex_t *mutex)
{
  if (mutex == nullptr) return 1;
  lock(*mutex);
  //SMP_PRINT("Spin locked %p\n", mutex);
  return 0;
}
int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
  if (mutex == nullptr) return 1;
  if (mutex) return 1;
  lock(*mutex);
  return 0;
}
int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
  if (mutex == nullptr) return 1;
  //SMP_PRINT("Spin unlocked %p\n", mutex);
  unlock(*mutex);
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
    //SMP_PRINT("Executing pthread_once routine %p\n", routine);
    executed = true;
    routine();
  }
  return 0;
}

int pthread_cond_wait(pthread_cond_t* cond, pthread_mutex_t* mutex)
{
  //SMP_PRINT("pthread_cond_wait\n");
  return 0;
}
int pthread_cond_broadcast(pthread_cond_t* cond)
{
  //SMP_PRINT("pthread_cond_broadcast\n");
  return 0;
}
int pthread_cond_signal(pthread_cond_t* cond)
{
  //SMP_PRINT("pthread_cond_signal %p\n", cond);
  return 0;
}
int pthread_cond_timedwait(pthread_cond_t* cond,
                           pthread_mutex_t* mutex,
                           const struct timespec* abstime)
{
  SMP_PRINT("pthread_cond_timedwait cond %p mutex %p\n", cond, mutex);
  return 0;
}
int pthread_cond_destroy(pthread_cond_t *cond)
{
  SMP_PRINT("pthread_cond_timedwait cond %p\n", cond);
  return 0;
}


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
  //SMP_PRINT("pthread_key_create: %p destructor %p\n", key, destructor);
  scoped_spinlock spinlock(key_lock);
  key_vec.push_back(nullptr);
  *key = key_vec.size()-1;
  return 0;
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr)
{
  PRINT("pthread_mutexattr_destroy(%p) = 0\n", attr);
  return 0;
}
int pthread_mutexattr_init(pthread_mutexattr_t *attr)
{
  PRINT("pthread_mutexattr_init(%p) = 0\n", attr);
  return 0;
}
int pthread_mutexattr_gettype(const pthread_mutexattr_t *__restrict attr, int *__restrict type)
{
  return 0;
}
int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type)
{
  return 0;
}

extern "C"
int nanosleep(const struct timespec *req, struct timespec *rem)
{
  PRINT("nanosleep(%p, %p) = -1\n", req, rem);
  return -1;
}
