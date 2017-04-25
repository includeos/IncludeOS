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

class Fiber_error : public std::runtime_error {
  using runtime_error::runtime_error;
};

class Fiber {

public:

  using init_func = delegate<void()>;
  using Stack_ptr = std::unique_ptr<char[]>;
  static constexpr int default_stack_size = 0x10000;
  static constexpr int redzone = 0;

  void start();
  void restart();

  Fiber(Fiber* parent, int stack_size, init_func func)
    : parent_{parent},
      id_{next_id_++},
      stack_size_{stack_size},
      stack_{Stack_ptr(new char[16 + stack_size_], std::default_delete<char[]> ())},
      stack_loc_{(void*)(uintptr_t(stack_.get() + stack_size_ ) &  ~ (uintptr_t)0xf)},
      func_{func}
  {}

  Fiber(Fiber* parent, init_func func)
    : Fiber(parent, default_stack_size, func)
  {}

  Fiber(init_func func)
    : Fiber(main_, default_stack_size, func)
  {}

  Fiber()
    : Fiber(main_, default_stack_size, nullptr)
  {}

  int id()
  { return id_; }

  Fiber* parent()
  { return parent_; }

  void resume();
  static void yield();

  static Fiber* main()
  { return main_; }

  bool suspended() {
    return suspended_;
  }

private:

  static Fiber* main_;
  static Fiber* current_;
  static int next_id_;

  Fiber* parent_;
  int id_ = next_id_ ++ ;
  int stack_size_ = default_stack_size;
  Stack_ptr stack_;
  void* stack_loc_;

  init_func func_ = nullptr;
  bool suspended_ = false;

};


#endif
