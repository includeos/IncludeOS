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

extern "C" void context_switch(Context::context_jump_func, uintptr_t stack);

void Context::create(context_jump_func func, size_t stack_size)
{
  char* stack_mem = new char[stack_size];
  uintptr_t start = (uintptr_t) (stack_mem+stack_size) & ~0xF;
  context_switch(func, start);
  delete[] stack_mem;
}
