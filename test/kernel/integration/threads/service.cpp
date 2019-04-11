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

#include <os>
#include <cassert>
#include <pthread.h>
#include <kernel/threads.hpp>

struct testdata
{
  int depth     = 0;
  const int max_depth = 40;
};

extern "C" {
  static void* thread_function1(void* data)
  {
    printf("Inside thread function1, x = %d\n", *(int*) data);
    thread_local int test = 2019;
    printf("test @ %p, test = %d\n", &test, test);
    return NULL;
  }
  static void* thread_function2(void* data)
  {
    printf("Inside thread function2, x = %d\n", *(int*) data);
    thread_local int test = 2020;
    printf("test @ %p, test = %d\n", &test, test);
    pthread_exit(NULL);
  }
  static void* recursive_function(void* tdata)
  {
    auto* data = (testdata*) tdata;
    data->depth++;
    printf("%ld: Thread depth %d / %d\n",
          kernel::get_thread()->tid, data->depth, data->max_depth);
    if (data->depth < data->max_depth)
    {
      pthread_t t;
      int res = pthread_create(&t, NULL, recursive_function, data);
      if (res < 0) {
        printf("Failed to create thread!\n");
        return NULL;
      }
    }
    printf("%ld: Thread exiting %d / %d\n",
           kernel::get_thread()->tid, data->depth, data->max_depth);
    data->depth--;
    pthread_t t = pthread_self();
    pthread_exit(&t);
    return NULL;
  }
}

void Service::start()
{
  int x = 666;
  int y = 777;
  pthread_t t;
  int res;
  printf("Calling pthread_create\n");
  res = pthread_create(&t, NULL, thread_function1, &x);
  if (res < 0) {
    printf("Failed to create thread!\n");
    return;
  }
  res = pthread_create(&t, NULL, thread_function2, &y);
  if (res < 0) {
    printf("Failed to create thread!\n");
    return;
  }
  printf("Now testing recursive threads...\n");
  static testdata rdata;
  recursive_function(&rdata);
  printf("SUCCESS\n");
  os::shutdown();
}
