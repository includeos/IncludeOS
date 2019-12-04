#pragma once
#include <array>
#include <deque>
#include <map>
#include <vector>

//#define THREADS_DEBUG 1
#ifdef THREADS_DEBUG
#define THPRINT(fmt, ...) kprintf(fmt, ##__VA_ARGS__)
#else
#define THPRINT(fmt, ...) /* fmt */
#endif

namespace kernel
{
  struct Thread {
    long    tid;
    Thread* parent;
    int     my_cpu;
    void*   my_tls;
    void*   my_stack;
    // for returning to this Thread
    void*   stored_stack = nullptr;
    void*   stored_nexti = nullptr;
    bool    yielded = false;
    // address zeroed when exiting
    void*   clear_tid = nullptr;
    // children, detached when exited
    std::vector<Thread*> children;

    void init(long tid, Thread* parent, void* stack);
    void yield();
    void exit();
    void suspend(void* ret_instr, void* ret_stack);
    void activate(void* newtls);
    void resume();
	void detach();
	void attach(Thread* parent);
  private:
    void libc_store_this();
  };

  struct ThreadManager
  {
	  std::map<long, kernel::Thread*> threads;
	  std::deque<Thread*> suspended;
	  Thread* main_thread = nullptr;

	  static ThreadManager& get() noexcept;
	  static ThreadManager& get(int cpu);

	  void migrate(long tid, int cpu);

	  void insert_thread(Thread* thread);
	  void erase_thread_safely(Thread* thread);

	  void erase_suspension(Thread* t);
	  void suspend(Thread* t) { suspended.push_back(t); }
	  Thread* wakeup_next();
  };

  inline Thread* get_thread()
  {
    Thread* thread;
    # ifdef ARCH_x86_64
        asm("movq %%fs:(0x10), %0" : "=r" (thread));
    # elif defined(ARCH_i686)
        asm("movq %%gs:(0x08), %0" : "=r" (thread));
    # else
        #error "Implement me?"
    # endif
    return thread;
  }

  Thread* get_thread(long tid); /* or nullptr */

  inline long get_tid() {
    return get_thread()->tid;
  }

  long get_last_thread_id() noexcept;

  void* get_thread_area();
  void  set_thread_area(void*);

  Thread* thread_create(Thread* parent, int flags, void* ctid, void* stack) noexcept;

  void resume(long tid);

  void setup_main_thread(long tid = 0) noexcept;
}

extern "C" {
  void __thread_yield();
}
