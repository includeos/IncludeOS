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

void scheduler1();
void scheduler2();
void scheduler3();

inline void* get_rsp() {
  void* stack = 0;
#if defined(ARCH_x86_64)
  asm volatile ("mov %%rsp, %0" :"=r"(stack));
#elif defined(ARCH_i686)
  asm volatile ("mov %%esp, %0" :"=r"(stack));
#endif
  return stack;
};


inline void* get_rbp() {
  void* base = 0;
#if defined(ARCH_x86_64)
  asm volatile ("mov %%rbp, %0" :"=r"(base));
#elif defined(ARCH_i686)
  asm volatile ("mov %%ebp, %0" :"=r"(base));
#endif

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

  INFO("Work","Worker %i completed", id);

}


void basic_test() {
  static int i = 0;
  i++;

  printf("\n **********  Basic_test %i  ********** \n", i);
  INFO("B1", "Main thread: %i", Fiber::main() ? Fiber::main()->id() : -1);
  INFO("B1", "test. backtrace: ");
  os::print_backtrace();
  printf("_____________________________________ \n\n");

}


void* posix_style(void* param) {

  printf("POSIX testz. Param: %p\n", param);
  if (not param)
    printf("POSIX test, empty parameter\n");
  else
    printf("Param string: %s\n", (char*)param);

  return nullptr;

}

void basic_yield() {
  static int i = 0;
  i++;

  printf("\n **********  Basic_yield %i  ********** \n", i);
  print_frame();
  INFO("Yielder", "Yielding now");

  Fiber::yield();

  INFO("Yielder", "RESUMED ");
  os::print_backtrace();
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
  Fiber basic{basic_yield};

  INFO("Sched2", "Running in fiber %i, starting fiber %i \n",
       Fiber::current()->id(), basic.id());

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
    threads.emplace_back(work);
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

std::string dangerous_string{"Hello danger"};
std::string empty_string;

std::string* strongly_typed(std::string* str) {
  INFO("Strong","Strongly typed param: %s", str->c_str());
  empty_string = "Danger is my middle name";

  Fiber::yield();

  INFO("Strong","Strongly typed resumed.");
  Expects(empty_string == "Danger is my middle name");

  return &empty_string;
}


void scheduler4(){
  printf("\n============================================== \n");
  printf("     SCHED 4  - run strongly typed fiber\n");
  printf("============================================== \n");

  Fiber strong{strongly_typed, &dangerous_string};
  strong.start();

  /** The line below throws Err_bad_cast: */
  /*
  char* ret = dangerous.ret<char*>();
  printf("Dangerous returned: %s \n", ret);
  */

  auto ret = strong.ret<std::string*>();
  INFO("Sched4", "Strongly typed fiber returned: %s", ret->c_str());

}

const char* posix_str = "Hello POSIX";



void print_int(int i) {
  INFO("Print_int","integer passed : %i \n", i);
}

long compute_int() {
  int i = 42;
  INFO("Compute_int","returning integer : %i \n", i);
  return i;
}

void Service::start()
{
  Expects(Fiber::main() == nullptr);
  Expects(Fiber::current() == nullptr);

  Fiber sched1 {scheduler1};
  Fiber sched2 {scheduler2};
  Fiber sched3 {scheduler3};


  printf("Main: %p, Current: %p \n" , Fiber::main(), Fiber::current());

  INFO("service", "Testing threading");
  print_frame();

  INFO("Service", "Param address %p, string: %s \n", posix_str, posix_str);
  os::print_backtrace();

  Fiber basic{basic_test};
  INFO("Service", "Starting basic test. rsp @ %p, fiber %i \n", get_rsp(), basic.id());
  print_frame();

  basic.start();


  INFO("Service", "Starting posix signature test. rsp @ %p", get_rsp());

  Fiber posix1{posix_style, (void*)posix_str};
  posix1.start();


  INFO("Service", "Starting sched1. rsp @ %p", get_rsp());
  print_frame();
  sched1.start();

  INFO("Service", "Starting sched2. rsp @ %p", get_rsp());
  print_frame();

  sched2.start();


  INFO("Service", "Starting sched3. rsp @ %p", get_rsp());
  print_frame();

  sched3.start();

  INFO("Service", "Starting sched4. rsp @ %p", get_rsp());

  Fiber sched4{scheduler4};
  sched4.start();

  INFO("Service", "Starting print_int. rsp @ %p", get_rsp());
  Fiber{print_int, 5}.start();

  INFO("Service", "Starting compute_int. rsp @ %p", get_rsp());
  Fiber compute{compute_int};
  compute.start();
  auto ret = compute.ret<long>();

  INFO("Service", "Computed long: %li", ret);


#ifdef INCLUDEOS_SMP_ENABLE
  if (SMP::cpu_count() > 1) {
    extern void fiber_smp_test();
    fiber_smp_test();
  } else {
    INFO("Service", "SMP test requires > 1 cpu's, found %i \n", SMP::cpu_count());
  }
#endif
  SMP_PRINT("Service done. rsp @ %p \n", get_rsp());
  SMP_PRINT("SUCCESS\n");
  exit(0);
}
