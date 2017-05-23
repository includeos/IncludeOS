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

#include <cstdio>
#include <delegate>

class Fiber;
extern "C" void fiber_jumpstarter(Fiber* f);

/** Exception: General error for fibers */
class Err_bad_fiber : public std::runtime_error {
  using runtime_error::runtime_error;
};

/** Exception: Trying to fetch an object of wrong type  **/
struct Err_bad_cast : public std::runtime_error {
  using runtime_error::runtime_error;
};


class Fiber {

public:

  using R_t = void*;
  using P_t = void*;
  using init_func = void*(*)(void*);
  using Stack_ptr = std::unique_ptr<char[]>;
  static constexpr int default_stack_size = 0x10000;
  static constexpr int redzone = 0;

  void start();
  void restart();

  template<typename R, typename P>
  Fiber(Fiber* parent, int stack_size, R(*func)(P), void* arg)
    : parent_{parent},
      id_{next_id_++},
      stack_size_{stack_size},
      stack_{Stack_ptr(new char[16 + stack_size_], std::default_delete<char[]> ())},
      stack_loc_{(void*)(uintptr_t(stack_.get() + stack_size_ ) &  ~ (uintptr_t)0xf)},
      type_return_{typeid(R)},
      type_param_{typeid(P)},
      func_{reinterpret_cast<init_func>(func)},
      param_{arg}
  {}

  template<typename R, typename P>
  Fiber(Fiber* parent, R(*func)(P))
    : Fiber(parent, default_stack_size, func, nullptr)
  {}

  template<typename R, typename P>
  Fiber(R(*func)(P), void* arg)
    : Fiber(main_, default_stack_size, func, arg)
  {}

  template<typename R, typename P>
  Fiber(R(*func)(P))
    : Fiber(main_, default_stack_size, func, nullptr)
  {}

  Fiber()
    : Fiber(init_func{nullptr})
  {}


  //
  // void versions
  //

  /** Fiber with void() function */
  Fiber(Fiber* parent, int stack_size, void(*func)())
    : parent_{parent},
      id_{next_id_++},
      stack_size_{stack_size},
      stack_{Stack_ptr(new char[16 + stack_size_], std::default_delete<char[]> ())},
      stack_loc_{(void*)(uintptr_t(stack_.get() + stack_size_ ) &  ~ (uintptr_t)0xf)},
      type_return_{typeid(void)},
      type_param_{typeid(void)},
      func_{reinterpret_cast<init_func>(func)}
  {}

  Fiber(void(*func)())
    : Fiber(main_, default_stack_size, func)
  {}


  /** Fiber with void(P) function. P must be storable in a void*  */
  template<typename P>
  Fiber(Fiber* parent, int stack_size, void(*func)(P), P par)
    : parent_{parent},
      id_{next_id_++},
      stack_size_{stack_size},
      stack_{Stack_ptr(new char[16 + stack_size_], std::default_delete<char[]> ())},
      stack_loc_{(void*)(uintptr_t(stack_.get() + stack_size_ ) &  ~ (uintptr_t)0xf)},
      type_return_{typeid(void)},
      type_param_{typeid(P)},
      func_{reinterpret_cast<init_func>(func)},
      param_{reinterpret_cast<void*>(par)}
  {
    static_assert(sizeof(P) <= sizeof(void*), "Invalid parameter size");
  }

  template<typename P>
  Fiber(void(*func)(P), P par)
    : Fiber(main_, default_stack_size, func, par)
  {}

  /** Fiber with R() function. R must be storable in a void*  */
  template<typename R>
  Fiber(Fiber* parent, int stack_size, R(*func)())
    : parent_{parent},
      id_{next_id_++},
      stack_size_{stack_size},
      stack_{Stack_ptr(new char[16 + stack_size_], std::default_delete<char[]> ())},
      stack_loc_{(void*)(uintptr_t(stack_.get() + stack_size_ ) &  ~ (uintptr_t)0xf)},
      type_return_{typeid(R)},
      type_param_{typeid(void)},
      func_{reinterpret_cast<init_func>(func)}
  {
    static_assert(sizeof(R) <= sizeof(void*), "Invalid return value size");
  }

  template<typename R>
  Fiber(R(*func)())
    : Fiber(main_, default_stack_size, func)
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

  template<typename R>
  R ret()
  {

    while (suspended_)
      resume();

    if (typeid(R) != type_return_)
      throw Err_bad_cast("Invalid return type for this funcion");

    // Probably exists some trick to allow narrowing
    return reinterpret_cast<R>(ret_);
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

  const std::type_info& type_return_;
  const std::type_info& type_param_;

  init_func func_ = nullptr;
  void* param_;
  void* ret_;

  bool suspended_ = false;

  friend void ::fiber_jumpstarter(Fiber* f);

};


#endif
