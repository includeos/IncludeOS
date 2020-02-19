#pragma once
#include <array>
#include <delegate>
#include <deque>
#include <map>
#include <vector>

//#define THREADS_DEBUG 1
#ifdef THREADS_DEBUG
#include <smp>
#define THPRINT(fmt, ...) { SMP::global_lock(); kprintf(fmt, ##__VA_ARGS__); SMP::global_unlock(); }
#else
#define THPRINT(fmt, ...) /* fmt */
#endif

namespace kernel
{
  struct Thread {
    int     tid;
    bool    migrated = false;
    bool    yielded = false;
    Thread* parent;
    void*   my_tls;
    void*   my_stack;
    // for returning to this Thread
    void*   stored_stack = nullptr;
    // address zeroed when exiting
    void*   clear_tid = nullptr;

	static Thread* create(Thread* parent, 
		long flags, void* ctid, void* ptid, void* stack);

    void init(int tid, Thread* parent, void* stack);
    void exit();
    void suspend(bool yielded, void* ret_stack);
    void set_tls(void* newtls);
    void resume();
	void detach();
	void attach(Thread* parent);
	void stack_push(uintptr_t value);
  private:
    void libc_store_this();
  };

  struct alignas(64) ThreadManager
  {
	  std::map<int, kernel::Thread*> threads;
	  std::deque<Thread*> suspended;
	  Thread* main_thread = nullptr;
	  Thread* last_thread = nullptr; // last inserted thread
	  Thread* next_thread = nullptr; // next yield-to thread

	  delegate<Thread*(ThreadManager&, Thread*)> on_new_thread = nullptr;

	  static ThreadManager& get();
	  static ThreadManager& get(int cpu);

	  Thread* detach(int tid);
	  void attach(Thread* thread);

	  bool has_thread(int tid) const noexcept { return threads.find(tid) != threads.end(); }
	  void insert_thread(Thread* thread);
	  void erase_thread_safely(Thread* thread);

	  void erase_suspension(Thread* t);
	  void suspend(Thread* t) { suspended.push_back(t); }
	  void yield_to(Thread* next);
	  Thread* wakeup_next();
  };

  inline Thread* get_thread()
  {
    Thread* thread;
    # ifdef ARCH_x86_64
        asm("movq %%fs:(48), %0" : "=r" (thread));
    # elif defined(ARCH_i686)
        asm("movq %%gs:(24), %0" : "=r" (thread));
    # elif defined(ARCH_aarch64)
        // TODO: fixme, find actual TP offset for aarch64 threads
        char* tp;
        asm("mrs %0, tpidr_el0" : "=r" (tp));
        thread = (Thread*) &tp[48];
    # else
        #error "Implement me"
    # endif
    return thread;
  }

  Thread* get_thread(int tid); /* or nullptr */

  inline int get_tid() {
    return get_thread()->tid;
  }

  Thread* get_last_thread();

  void* get_thread_area();
  void  set_thread_area(void*);

  void resume(int tid);

  Thread* setup_main_thread(int cpu = 0, int tid = 0);
}

extern "C" {
  void __thread_yield();
}
