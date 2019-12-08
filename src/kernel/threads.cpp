#include <kernel/threads.hpp>
#include <arch/x86/cpu.hpp>
#include <smp>
#include <cassert>
#include <pthread.h>
#include <kprint>

extern "C" {
  void __thread_yield();
  void __thread_restore(void* stack);
  void __clone_return(void* stack);
  long __migrate_resume(void* stack);
  long syscall_SYS_set_thread_area(void* u_info);
}
static constexpr bool PRIORITIZE_PARENT = true;

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

  std::vector<ThreadManager> thread_managers;
  SMP_RESIZE_GCTOR(thread_managers);
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
  void Thread::stack_push(uintptr_t value)
  {
	this->my_stack = (void*) ((uintptr_t) this->my_stack - sizeof(uintptr_t));
	*((uintptr_t*) this->my_stack) = value;
  }
  void Thread::libc_store_this()
  {
    auto* s = (libc_internal*) this->my_tls;
    s->kthread = this;
  }
  void Thread::set_tls(void* newtls)
  {
    this->my_tls = newtls;
    // store ourselves in the guarded libc structure
    this->libc_store_this();
  }

  void Thread::suspend(bool yielded, void* ret_stack)
  {
	  THPRINT("Thread %ld suspended, yielded=%d stack=%p\n",
              this->tid, yielded, ret_stack);
	  this->yielded = yielded;
      this->stored_stack = ret_stack;
      // add to suspended (NB: can throw)
      ThreadManager::get().suspend(this);
  }

  void Thread::exit()
  {
    const bool exiting_myself = (get_thread() == this);
	auto& tman = ThreadManager::get();
    assert(this->parent != nullptr);
    // detach children
    for (auto* child : this->children) {
        child->parent = tman.main_thread;
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
	tman.erase_thread_safely(this);
    // free thread resources
    delete this;
    // NOTE: cannot deref this after this
    if (exiting_myself)
    {
		if constexpr (PRIORITIZE_PARENT) {
			// only resume this thread if its on this CPU
			if (tman.has_thread(next->tid)) {
				tman.erase_suspension(next);
				next->resume();
			}
		}
		next = tman.wakeup_next();
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
      set_thread_area(this->my_tls);
      THPRINT("Returning to tid=%ld tls=%p stack=%p thread=%p\n",
            this->tid, this->my_tls, this->stored_stack, get_thread());
      // NOTE: the RAX return value here is CHILD thread id, not this
      if (this->yielded == false) {
          __clone_return(this->stored_stack);
      }
      else {
          this->yielded = false;
          __thread_restore(this->stored_stack);
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

  Thread* setup_main_thread(long tid)
  {
		int stack_value;
		if (tid == 0)
		{
			core0_main_thread.init(0, nullptr, (void*) &stack_value);
			ThreadManager::get(0).insert_thread(&core0_main_thread);
			// allow exiting in main thread
			core0_main_thread.set_tls(get_thread_area());
			// make threadmanager0 use this main thread
			// NOTE: don't use SMP-aware function here
			ThreadManager::get(0).main_thread = &core0_main_thread;
			return &core0_main_thread;
		}
		else
		{
			auto* main_thread = get_thread(tid);
			assert(main_thread->parent == nullptr && "Must be a detached thread");
			ThreadManager::get().main_thread = main_thread;
			return main_thread;
		}
  }
  void setup_automatic_thread_multiprocessing()
  {
	  ThreadManager::get().on_new_thread =
	  [] (ThreadManager& man, Thread* thread) -> Thread* {
		  auto* kthread = man.detach(thread->tid);
		  SMP::add_task(
		  [kthread] () {
#ifdef THREADS_DEBUG
			  SMP::global_lock();
			  THPRINT("CPU %d resuming migrated thread %ld (stack=%p)\n",
			  		SMP::cpu_id(), kthread->tid,
					(void*) kthread->my_stack);
			  SMP::global_unlock();
#endif
			  // attach this thread on this core
			  ThreadManager::get().attach(kthread);
			  // resume kthread after yielding this thread
			  ThreadManager::get().finish_migration_to(kthread);
			  // NOTE: returns here!!
		  }, nullptr);
		  // signal that work exists in the global queue
		  SMP::signal();
		  // indicate that the thread has been detached
		  return nullptr;
	  };
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
      if (it != threads.end()) return it->second;
	  return nullptr;
  }

  void resume(long tid)
  {
	  auto* thread = get_thread(tid);
	  if (thread != nullptr) {
	  	thread->resume();
		__builtin_unreachable();
	  }
	  THPRINT("Could not resume thread, missing: %ld\n", tid);
	  assert(thread && "Could not find thread id");
  }

  Thread* ThreadManager::detach(long tid)
  {
	  auto* thread = get_thread(tid);
	  // can't migrate missing thread
	  assert(thread != nullptr && "Could not find given thread id");
	  // can't migrate the main thread
	  assert(thread != ThreadManager::get().main_thread);
	  // can't migrate the thread you are in
	  auto* current = get_thread();
	  assert(current != thread && "Can't migrate current thread");
	  // remove from old thread manager
	  if (thread->parent != nullptr) {
		  thread->detach();
	  }
	  this->erase_thread_safely(thread);
	  this->erase_suspension(thread);
	  // return the free, detached thread
	  return thread;
  }
  void ThreadManager::attach(Thread* thread)
  {
	  // insert into new thread manager
	  this->insert_thread(thread);
	  // attach this thread to the managers main thread
	  if (this->main_thread) {
		thread->attach(this->main_thread);
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
  void ThreadManager::finish_migration_to(Thread* thread)
  {
	  // special migration-yield to next thread
	  this->next_migration_thread = thread;
	  __thread_yield(); // NOTE: function returns!!
  }
}

// called from __thread_yield assembly, cannot return
extern "C"
void __thread_suspend_and_yield(void* stack)
{
	auto& man = kernel::ThreadManager::get();
	// don't go through the ardous yielding process when alone
	if (man.suspended.empty() && man.next_migration_thread == nullptr) {
		THPRINT("Nothing to yield to. Returning... thread=%p stack=%p\n",
				kernel::get_thread(), stack);
		return;
	}
	// suspend current thread (yielded)
	auto* thread = kernel::get_thread();
	thread->suspend(true, stack);

	if (man.next_migration_thread == nullptr)
	{
		// resume some other thread
		auto* next = man.wakeup_next();
		// resume next thread
		next->resume();
	}
	else
	{
		// resume migrated thread
		auto* kthread = man.next_migration_thread;
		man.next_migration_thread = nullptr;
		// resume the thread on this core
		kernel::set_thread_area(kthread->my_tls);
		assert(kernel::get_thread() == kthread);

		__migrate_resume(kthread->my_stack);
	}
	__builtin_unreachable();
}
