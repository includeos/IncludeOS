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

#include <kernel/context.hpp>
#include <cstdint>

extern "C" void __context_switch(uintptr_t stack);

static Context::context_func destination;
extern "C"
void __context_switch_delegate()
{
  destination();
}

void Context::jump(void* location, context_func func)
{
  // store so we can call it later
  destination = func;
  // switch to stack from @location
  __context_switch((uintptr_t) location);
}

void Context::create(unsigned stack_size, context_func func)
{
  // store so we can call it later
  destination = func;
  // create and switch to new stack
  char* stack_mem = new char[stack_size];
  assert(stack_mem);
  // aligned to 16 byte boundary
  uintptr_t start = (uintptr_t) (stack_mem+stack_size) & ~0xF;
  __context_switch(start);
  delete[] stack_mem;
}
