
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
