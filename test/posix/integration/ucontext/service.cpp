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
#include <ucontext.h>

ucontext_t foo_context, bar_context, main_context;
unsigned int bar_context_var = -1;
unsigned int foo_context_var = -1;

void bar(int argc)
{
  printf("Successfuly jumped into 'bar' context\n");

  Expects(argc == 0);

  bar_context_var = 0xDEADBEEF;

  printf("'bar' context returning successfuly\n");
}

void foo(int argc, int arg1, int arg2)
{
  printf("Successfuly jumped into 'foo' context\n");

  Expects(argc == 2);
  Expects(arg1 == -2414);
  Expects(arg2 == ~0);

  foo_context_var = 0xFEEDDEAD;


  volatile bool swapped = false;

  if(!swapped) {
    swapped = true;
    Expects(swapcontext(&foo_context, &bar_context) != -1);
  }

  Expects(bar_context_var == 0xDEADBEEF);

  printf("'foo' context returning succesfuly\n");
}

void Service::start(const std::string&)
{
  INFO("ucontext_t", "Testing POSIX ucontext_t");

  uint8_t foo_stack[1024], bar_stack[1024];

  Expects(getcontext(&foo_context) != -1);
  foo_context.uc_link = &main_context;
  foo_context.uc_stack.ss_sp = foo_stack + 1024;
  foo_context.uc_stack.ss_size = 1024;
  makecontext(&foo_context, (void(*)())foo, 2, -2414, ~0);

  Expects(getcontext(&bar_context) != -1);
  bar_context.uc_link = &foo_context;
  bar_context.uc_stack.ss_sp = bar_stack + 1024;
  bar_context.uc_stack.ss_size = 1024;
  makecontext(&bar_context, (void(*)())bar, 0);

  volatile bool swapped = false;
  getcontext(&main_context);

  if(!swapped) {
    swapped = true;
    Expects(swapcontext(&main_context, &foo_context) != -1);
  }

  Expects(foo_context_var == 0xFEEDDEAD);

  INFO("ucontext ","SUCCESS");
}
