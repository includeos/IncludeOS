#pragma once
#include <array>
#include <vector>

//#define THREADS_DEBUG 1
#ifdef THREADS_DEBUG
#define THPRINT(fmt, ...) kprintf(fmt, ##__VA_ARGS__)
#else
#define THPRINT(fmt, ...) /* fmt */
#endif

namespace kernel
{
  struct thread_t {
    thread_t* self;
    thread_t* parent = nullptr;
    int64_t  tid;
    void*    my_tls;
    void*    my_stack;
    // for returning to this thread
    void*    stored_stack = nullptr;
    void*    stored_nexti = nullptr;
    bool     yielded = false;
    // address zeroed when exiting
    void*    clear_tid = nullptr;
    // children, detached when exited
    std::vector<thread_t*> children;

    void init(int tid);
    void yield();
    void exit();
    void suspend(void* ret_instr, void* ret_stack);
    void activate(void* newtls);
    void resume();
  private:
    void store_return(void* ret_instr, void* ret_stack);
    void libc_store_this();
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

  thread_t* get_thread(int64_t tid); /* or nullptr */

  inline int64_t get_tid() {
    return get_thread()->tid;
  }

  void* get_thread_area();

  thread_t* thread_create(thread_t* parent, int flags, void* ctid, void* stack) noexcept;

  void setup_main_thread() noexcept;
}

extern "C" {
  void __thread_yield();
}
