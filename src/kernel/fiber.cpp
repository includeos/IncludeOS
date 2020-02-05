// This file is a part of the IntcludeOS unikernel - www.includeos.org

//#define SMP_DEBUG 1
#include <kernel/fiber.hpp>
#include <common> // assert-based Exepcts/Ensures
#include <cstdint>
#include <memory>
#include <smp>

// Default location for previous stack. Asm will always save a pointer.
#ifdef INCLUDEOS_SMP_ENABLE
std::atomic<int> Fiber::next_id_{0};
#else
int Fiber::next_id_{0};
#endif

std::vector<Fiber*> Fiber::main_ = {{nullptr}};
std::vector<Fiber*> Fiber::current_ {{nullptr}};

extern "C" {
  void __fiber_jumpstart(volatile void* th_stack, volatile Fiber* f, volatile void* parent_stack);
  void __fiber_yield(volatile void* stack, volatile void* parent_stack);

  /** Jumpstart a fiber. Called from __fiber_jumpstart (assembly) */
  void fiber_jumpstarter(Fiber* f)
  {

    Expects(f);
    f->ret_= f->func_(f->param_);

    // Last stackframe before switching back. Done.
    PER_CPU(Fiber::current_) = f->parent_ ? f->parent_ : nullptr;
    f->done_ = true;
  }
}



void Fiber::start() {

  if (not func_)
    throw Err_bad_fiber("Can't start fiber without a function");

  // Become main if none exists
  if (not PER_CPU(main_))
    PER_CPU(main_) = this;

  // Suspend current running fiber if any
  if (PER_CPU(current_)) {
    PER_CPU(current_) -> suspended_ = true;
    make_parent(PER_CPU(current_));
  }

  // Become current
  PER_CPU(current_) = this;

  started_ = true;
  running_ = true;

  // Switch stacks and call function
  __fiber_jumpstart(stack_loc_, this, &(parent_stack_));

  // Returns here after first yield / final return

  if (PER_CPU(main_) == this)
    PER_CPU(main_) = nullptr;

  Expects(PER_CPU(current_) != this);
}

void Fiber::yield() {

  auto* from = PER_CPU(current_);
  Expects(from);
  auto* into = PER_CPU(current_)->parent_;
  Expects(into);
  Expects(into->suspended());
  Expects(into->stack_loc_);
  Expects(from->stack_loc_);
  Expects(not into->done_);

  from->suspended_ = true;
  from->running_ = false;

  PER_CPU(current_) = into;

  __fiber_yield(from->parent_stack_, &(from->stack_loc_));

};

void Fiber::resume()
{

  if (not suspended_ or done_ or func_ == nullptr)
    return;

  Expects(PER_CPU(current_));

  make_parent(PER_CPU(current_));

  PER_CPU(current_) = this;
  suspended_ = false;
  running_ = true;
  parent_->suspended_ = true;
  parent_->running_ = false;

  Expects(stack_loc_ > stack_.get() and stack_loc_ < stack_.get() + stack_size_);
  Expects(not done_);

  __fiber_yield(stack_loc_, &(parent_stack_));

  // Returns here after yield

}
