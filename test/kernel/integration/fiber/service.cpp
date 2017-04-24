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

#include <os>
#include <vector>
#include <kernel/fiber.hpp>

//extern void print_backtrace();

void scheduler1();
void scheduler2();
void scheduler3();

Fiber sched1 {scheduler1};
Fiber sched2 {scheduler2};
Fiber sched3 {scheduler3};

Fiber* Fiber::main_ = nullptr;
Fiber* Fiber::current_ = nullptr;

inline void* get_rsp() {
  void* stack = 0;
  asm volatile ("mov %%rsp, %0" :"=r"(stack));
  return stack;
};


inline void* get_rbp() {
  void* base = 0;
  asm volatile ("mov %%rbp, %0" :"=r"(base));
  return base;
};


inline void print_frame()
{
  INFO("Frame","(rbp) %p - %p (rsp) ", get_rbp(), get_rsp());
}

static int work_id = 0;

void work() {

  int id = work_id ++;

  INFO("Work","Thread func. %i starting. Stack @ %p", id, get_rsp());
  for (int i = 0; i < 10; i++) {
    INFO("Work","Worker %i iteration %i", id, i);
    Fiber::yield();
    INFO("Work","Worker %i resumed", id);
  }
}


void basic_test() {
  static int i = 0;
  i++;

  printf("\n **********  Basic_test %i  ********** \n", i);
  INFO("B1", "test. backtrace: ");
  print_backtrace();
  printf("_____________________________________ \n\n");
}

void basic_yield() {
  static int i = 0;
  i++;

  printf("\n **********  Basic_yield %i  ********** \n", i);
  print_frame();
  INFO("Yielder", "Yielding now");

  Fiber::yield();

  INFO("Yielder", "RESUMED ");
  print_backtrace();
  print_frame();
  INFO("Yielder", "EXIT ");
  printf("_____________________________________ \n\n");

}


std::vector<Fiber> threads;

void scheduler1(){
  printf("\n============================================== \n");
  printf("       SCHED 1  - thread from thread \n");
  printf("============================================== \n");
  print_frame();
  INFO("Scheduler1", "Started. Initializing thread.");

  // Add some stack data and make sure it surives a stack switch
  int buflen = 100;
  char arr[buflen];
  snprintf(arr, 100, "I'm the first scheduler, buflen %i", buflen);
  INFO("Scheduler1","My string: %s ", arr);

  Fiber basic{basic_test};
  basic.start();
  print_frame();

  INFO("Scheduler1", "Basic thread returned. My string: ");
  INFO("Scheduler1","My string: %s ", arr);
  INFO("Scheduler1", "Completed. Returning to service.");

  printf("______________________________________________ \n");
}


void scheduler2(){
  printf("\n============================================== \n");
  printf("     SCHED 2  - schedule yielding thread \n");
  printf("============================================== \n");
  print_frame();
  Fiber basic{&sched2, basic_yield};

  Expects(Fiber::main() == &sched2);
  basic.start();

  if (basic.suspended()) {
    INFO("Sched2", "Thread %i YIELDED. Resuming ", basic.id());
    print_frame();
    basic.resume();

  } else {
    INFO("Sched2","No yield, just finish.");
  }

  INFO("Sched2","DONE");
  print_frame();
  printf("______________________________________________ \n");

}

void scheduler3 () {
  printf("\n============================================== \n");
  printf("     SCHED 3  - schedule several workers \n");
  printf("============================================== \n");

  const int N = 5;

  INFO("Scheduler", "Started. Initializing %i threads", N);

  for (int i = 0; i < N; i++) {
    threads.emplace_back(Fiber{work});
  }

  for (auto& thread : threads) {
    thread.start();
  }

  INFO ("Scheduler", "All threads started. Resuming in turn.");


  for (int i = 0; i < 40; i++) {
    for (auto& thread : threads) {
      thread.resume();
    }
  }

  INFO("Scheduler", "Done ");
}


void Service::start()
{
  INFO("service", "Testing threading");
  print_frame();

  INFO("Service","Init backtrace: \n\t* 0: %p\n\t* 1: %p \n\t* 2: %p\n\t* 3:%p\n",
       __builtin_return_address(0),
       __builtin_return_address(1),
       __builtin_return_address(2),
       __builtin_return_address(3));

  print_backtrace();


  INFO("Service", "Starting basic test. rsp @ %p", get_rsp());
  print_frame();


  Fiber basic{basic_test};
  basic.start();


  INFO("Service", "Starting sched1. rsp @ %p", get_rsp());
  print_frame();

  sched1.start();

  INFO("Service", "Starting sched2. rsp @ %p", get_rsp());
  print_frame();

  sched2.start();


  INFO("Service", "Starting sched3. rsp @ %p", get_rsp());
  print_frame();

  sched3.start();


  printf("\n");
  INFO("Service", "Done. rsp @ %p", get_rsp());
  print_frame();
  exit(0);

}
