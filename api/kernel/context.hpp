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
#ifndef KERNEL_CONTEXT_HPP
#define KERNEL_CONTEXT_HPP

#include <delegate>

struct Context
{
  typedef delegate<void()> context_func;

  // create and switch to a new stack with size @stack_size and call func
  static void create(unsigned stack_size, context_func);
  // switch to a pre-allocated location and call func
  static void jump(void* location, context_func);
};

#endif
