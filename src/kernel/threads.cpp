#include <kernel/threads.hpp>
#include <arch/x86/cpu.hpp>
#include <smp>
#include <cassert>
#include <deque>
#include <pthread.h>
#include <kprint>

extern "C" {
  void __thread_yield();
  void __thread_restore(void* nexti, void* stack);
  void __clone_return(void* nexti, void* stack);
  long syscall_SYS_set_thread_area(void* u_info);
}

struct libc_internal {
  void* self;
  void* dtv;
  kernel::Thread* kthread;
};

namespace kernel
{
  static long thread_counter = 1;
  static Thread core0_main_thread;

  inline long generate_new_thread_id() noexcept {
	  return __sync_fetch_and_add(&thread_counter, 1);
  }
  long get_last_thread_id() noexcept {
	  return thread_counter-1;
  }

  SMP::Array<ThreadManager> thread_managers;
  ThreadManager& ThreadManager::get() noexcept {
	  return PER_CPU(thread_managers);
  }
  ThreadManager& ThreadManager::get(int cpu) {
	  return thread_managers.at(cpu);
  }

  void Thread::init(long tid, Thread* parent, void* stack)
  {
    this->tid = tid;
	this->parent = parent;
	this->my_stack = stack;
	if (this->parent) {
		this->parent->children.push_back(this);
	}

  }

  void Thread::libc_store_this()
  {
    auto* s = (libc_internal*) this->my_tls;
    s->kthread = this;
  }
  void Thread::activate(void* newtls)
  {
    this->my_tls = newtls;
    // store ourselves in the guarded libc structure
    this->libc_store_this();
    set_thread_area(this->my_tls);
  }

  void Thread::suspend(void* ret_instr, void* ret_stack)
  {
	  THPRINT("Thread %ld storing return point %p with stack %p\n",
              this->tid, ret_instr, ret_stack);
      this->stored_nexti = ret_instr;
      this->stored_stack = ret_stack;
      // add to suspended (NB: can throw)
      ThreadManager::get().suspend(this);
  }
  void Thread::yield()
  {
      // resume a waiting thread
	  auto* next = ThreadManager::get().wakeup_next();
      // resume next thread
      this->yielded = true;
      next->resume();
  }

  void Thread::exit()
  {
    const bool exiting_myself = (get_thread() == this);
    assert(this->parent != nullptr);
    // detach children
    for (auto* child : this->children) {
        child->parent = ThreadManager::get().main_thread;
    }
    // temporary copy of parent thread pointer
    auto* next = this->parent;
    // remove myself from parent
    this->detach();
    // CLONE_CHILD_CLEARTID: set userspace TID value to zero
    if (this->clear_tid) {
        THPRINT("Clearing child value at %p\n", this->clear_tid);
        *(pid_t*) this->clear_tid = 0;
    }
    // delete this thread
	ThreadManager::get().erase_thread_safely(this);
    // free thread resources
    delete this;
    // resume parent thread
    if (exiting_myself)
    {
        ThreadManager::get().erase_suspension(next);
        next->resume();
    }
  }
  void Thread::detach()
  {
	  assert(this->parent != nullptr);
	  auto& pcvec = this->parent->children;
      for (auto it = pcvec.begin(); it != pcvec.end(); ++it) {
          if (*it == this) {
              pcvec.erase(it);
			  break;
          }
      }
	  this->parent = nullptr;
  }
  void Thread::attach(Thread* parent)
  {
	  this->parent = parent;
	  parent->children.push_back(this);
  }

  void Thread::resume()
  {
      THPRINT("Returning to tid=%ld tls=%p nexti=%p stack=%p\n",
            this->tid, this->my_tls, this->stored_nexti, this->stored_stack);
      // NOTE: the RAX return value here is CHILD thread id, not this
      if (this->yielded == false) {
          set_thread_area(this->my_tls);
          __clone_return(this->stored_nexti, this->stored_stack);
      }
      else {
          this->yielded = false;
          set_thread_area(this->my_tls);
          __thread_restore(this->stored_nexti, this->stored_stack);
      }
      __builtin_unreachable();
  }

  Thread* thread_create(Thread* parent, int flags,
                          void* ctid, void* stack) noexcept
  {
    const long tid = generate_new_thread_id();
    try {
      auto* thread = new struct Thread;
      thread->init(tid, parent, stack);

      // flag for write child TID
      if (flags & CLONE_CHILD_SETTID) {
          *(pid_t*) ctid = thread->tid;
      }
      if (flags & CLONE_CHILD_CLEARTID) {
          thread->clear_tid = ctid;
      }

	  ThreadManager::get().insert_thread(thread);
      return thread;
    }
    catch (...) {
      return nullptr;
    }
  }

  void setup_main_thread(long tid) noexcept
  {
		int stack_value;
		if (tid == 0)
		{
			core0_main_thread.init(0, nullptr, (void*) &stack_value);
			// allow exiting in main thread
			core0_main_thread.activate(get_thread_area());
			// make threadmanager0 use this main thread
			// NOTE: don't use SMP-aware function here
			ThreadManager::get(0).main_thread = &core0_main_thread;
		}
		else
		{
			auto* main_thread = get_thread(tid);
			assert(main_thread->parent == nullptr);
			ThreadManager::get().main_thread = main_thread;
		}
  }

  void* get_thread_area()
  {
# ifdef ARCH_x86_64
    return (void*) x86::CPU::read_msr(IA32_FS_BASE);
# else
    #error "Implement me"
# endif
  }

  void set_thread_area(void* new_area) {
	  syscall_SYS_set_thread_area(new_area);
  }

  Thread* get_thread(long tid) {
	  auto& threads = ThreadManager::get().threads;
      auto it = threads.find(tid);
      if (it == threads.end()) return nullptr;
      return it->second;
  }

  void resume(long tid)
  {
	  auto* thread = get_thread(tid);
	  assert(thread);
	  thread->resume();
  }

  void ThreadManager::migrate(long tid, int cpu)
  {
	  auto* thread = get_thread(tid);
	  // can't migrate missing thread
	  assert(thread != nullptr && "Could not find given thread id");
	  // can't migrate the main thread
	  assert(thread != ThreadManager::get().main_thread);
	  // can't migrate the thread you are in
	  auto* current = get_thread();
	  assert(current != thread && "Can't migrate current thread");
	  // start migration
	  if (thread->parent != nullptr) {
		  thread->detach();
	  }
	  this->erase_thread_safely(thread);
	  auto& tman = ThreadManager::get(cpu);
	  tman.insert_thread(thread);
	  // attach this thread to the managers main thread
	  if (tman.main_thread) {
	  	thread->attach(tman.main_thread);
	  }
  }
  void ThreadManager::insert_thread(Thread* thread)
  {
	  threads.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(thread->tid),
			std::forward_as_tuple(thread));
  }
  void ThreadManager::erase_thread_safely(Thread* thread)
  {
	  assert(thread != nullptr);
	  auto it = threads.find(thread->tid);
	  assert(it != threads.end());
	  assert(it->second == thread);
	  threads.erase(it);
  }
  Thread* ThreadManager::wakeup_next()
  {
	  assert(!suspended.empty());
	  auto* next = suspended.front();
	  suspended.pop_front();
	  return next;
  }
  void ThreadManager::erase_suspension(Thread* t)
  {
      for (auto it = suspended.begin(); it != suspended.end();)
      {
          if (*it == t) {
              it = suspended.erase(it);
          }
          else {
              ++it;
          }
      }
  }
}

extern "C"
void __thread_suspend_and_yield(void* next_instr, void* stack)
{
    // don't go through the ardous yielding process when alone
    if (kernel::ThreadManager::get().suspended.empty()) {
		THPRINT("Nothing to yield to. Returning... nexti=%p stack=%p\n",
              next_instr, stack);
		return;
	}
    // suspend current thread
    auto* thread = kernel::get_thread();
    thread->suspend(next_instr, stack);
    // resume some other thread
    thread->yield();
}

extern "C"
long syscall_SYS_sched_setscheduler(pid_t /*pid*/, int /*policy*/,
                          const struct sched_param* /*param*/)
{
  return 0;
}
