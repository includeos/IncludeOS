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
    long    tid;
    Thread* parent;
    void*   my_tls;
    void*   my_stack;
    // for returning to this Thread
    void*   stored_stack = nullptr;
    bool    yielded = false;
    // address zeroed when exiting
    void*   clear_tid = nullptr;
    // children, detached when exited
    std::vector<Thread*> children;

    void init(long tid, Thread* parent, void* stack);
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

  struct ThreadManager
  {
	  std::map<long, kernel::Thread*> threads;
	  std::deque<Thread*> suspended;
	  Thread* main_thread = nullptr;
	  Thread* next_migration_thread = nullptr;

	  delegate<Thread*(ThreadManager&, Thread*)> on_new_thread = nullptr;

	  static ThreadManager& get() noexcept;
	  static ThreadManager& get(int cpu);

	  Thread* detach(long tid);
	  void attach(Thread* thread);

	  bool has_thread(long tid) const noexcept { return threads.find(tid) != threads.end(); }
	  void insert_thread(Thread* thread);
	  void erase_thread_safely(Thread* thread);

	  void erase_suspension(Thread* t);
	  void suspend(Thread* t) { suspended.push_back(t); }
	  void finish_migration_to(Thread* next);
	  Thread* wakeup_next();
  };

  inline Thread* get_thread()
  {
    Thread* thread;
    # ifdef ARCH_x86_64
        asm("movq %%fs:(0x10), %0" : "=r" (thread));
    # elif defined(ARCH_i686)
        asm("movq %%gs:(0x08), %0" : "=r" (thread));
    # elif defined(ARCH_aarch64)
        // TODO: fixme, find actual TP offset for aarch64 threads
        char* tp;
        asm("mrs %0, tpidr_el0" : "=r" (tp));
        thread = (Thread*) &tp[0x10];
    # else
        #error "Implement me"
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

  Thread* thread_create(Thread* parent, int flags, void* ctid, void* ptid, void* stack) noexcept;

  void resume(long tid);

  Thread* setup_main_thread(long tid = 0);
  void setup_automatic_thread_multiprocessing();
}

extern "C" {
  void __thread_yield();
}
