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

#include <kernel/fiber.hpp>
#include <gsl/gsl>
#include <cstdint>
#include <memory>

void* previous_stack_ = nullptr;
Fiber* Fiber::main_ = nullptr;
Fiber* Fiber::current_ = nullptr;


extern "C" {
  void __fiber_jumpstart(void* th_stack, Fiber* f, void* parent_stack);
  void __fiber_yield(void* stack, void* parent_stack);

  void fiber_jumpstarter(Fiber* f)
  {
    f->ret_= f->func_(f->param_);
  }
}


int Fiber::next_id_ = 0;

void Fiber::start() {

  if (not main_)  {
    Expects(parent_ == nullptr);
    main_ = this;
  }

  if (not func_)
    throw Err_bad_fiber("Can't start fiber without a function");

  current_ = this;

  // Switch stack + call thread initialize
  void* prev_stack = previous_stack_;

  if (parent_)
    prev_stack = &(parent_->stack_loc_);

  __fiber_jumpstart(stack_loc_, this, prev_stack);

  if (main_ == this)
    main_ = nullptr;

}

void Fiber::yield() {

  Fiber* child = current_;
  Fiber* parent = child->parent();

  child->suspended_ = true;
  current_ = parent;

  __fiber_yield(parent->stack_loc_, &(child->stack_loc_));

};

void Fiber::resume()
{

  if (not suspended_)
    return;

  Fiber* parent = current_;
  current_ = this;
  suspended_ = false;

  Expects(stack_loc_ > stack_.get() and stack_loc_ < stack_.get() + stack_size_);

  __fiber_yield(stack_loc_, &(parent->stack_loc_));
}
