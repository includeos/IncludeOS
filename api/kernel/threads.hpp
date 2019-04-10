#pragma once
#include <array>

namespace kernel
{
  struct thread_t {
    thread_t* self;
    int64_t  tid;
    void*    ret_instr;
    void*    ret_stack;
    uint64_t ret_tls;

    void init(int tid);
  };

  inline thread_t* get_thread()
  {
    thread_t* thread;
    # ifdef ARCH_x86_64
        asm("movq %%fs:(0x10), %0" : "=r" (thread));
    # elif defined(ARCH_i686)
        asm("movq %%gs:(0x08), %0" : "=r" (thread));
    # else
        #error "Implement me?"
    # endif
    return thread;
  }

  thread_t* thread_create() noexcept;
  void thread_exit() noexcept;

  void setup_main_thread() noexcept;
}
