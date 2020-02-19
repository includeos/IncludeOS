#include <kernel/threads.hpp>
#include <smp>
#include <common>
#include <sched.h>
#include <kprint>
#ifdef ARCH_x86_64
#include <arch/x86/cpu.hpp>
#endif

extern "C" {
  void __thread_yield();
  void __thread_restore(void* stack);
  void __clone_return(void* stack);
  long __migrate_resume(void* stack);
  long syscall_SYS_set_thread_area(void* u_info);
}
static constexpr bool PRIORITIZE_PARENT = true;

namespace os {
	__attribute__((noreturn))
	extern void panic(const char* why) noexcept;
}

struct libc_internal {
  void* self;
  void* dtv;
  void* prev;
  void* next;
  void* sysinfo;
  void* canary;
  // this is what we are reduced to... for shame!
  kernel::Thread* kthread;
};

namespace kernel
{
  static int thread_counter = 1;
  static Thread core0_main_thread;

  inline int generate_new_thread_id() noexcept {
	  return __sync_fetch_and_add(&thread_counter, 1);
  }

  static std::vector<ThreadManager> thread_managers;
  SMP_RESIZE_EARLY_GCTOR(thread_managers);
  ThreadManager& ThreadManager::get() {
	  return PER_CPU(thread_managers);
  }
  ThreadManager& ThreadManager::get(int cpu) {
	  return thread_managers.at(cpu);
  }

  Thread* get_last_thread() {
	  return ThreadManager::get().last_thread;
  }

  void Thread::init(int tid, Thread* parent, void* stack)
  {
    this->tid = tid;
	this->parent = parent;
	this->my_stack = stack;
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
	  THPRINT("CPU %d: Thread %d suspended, yielded=%d stack=%p\n",
              SMP::cpu_id(), this->tid, yielded, ret_stack);
	  this->yielded = yielded;
      this->stored_stack = ret_stack;
      // add to suspended (NB: can throw)
      ThreadManager::get().suspend(this);
  }

  void Thread::exit()
  {
    const bool exiting_myself = (get_thread() == this);
	auto& tman = ThreadManager::get();
    Expects(this->parent != nullptr);
    // temporary copy of parent thread pointer
    auto* next = this->parent;
    // remove myself from parent
    this->detach();
    // CLONE_CHILD_CLEARTID: set userspace value to zero
    if (this->clear_tid) {
        THPRINT("Clearing child value at %p for tls=%p\n",
                this->clear_tid, this->my_tls);
        *(int*) this->clear_tid = 0;
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
	  Expects(this->parent != nullptr);
	  this->parent = nullptr;
  }
  void Thread::attach(Thread* parent)
  {
	  this->parent = parent;
  }

  void Thread::resume()
  {
      set_thread_area(this->my_tls);
      THPRINT("CPU %d: Returning to tid=%d tls=%p stack=%p thread=%p\n",
            SMP::cpu_id(), this->tid, this->my_tls, this->stored_stack, get_thread());
      Expects(kernel::get_thread() == this);
      // NOTE: the RAX return value here is CHILD thread id, not this
      if (UNLIKELY(this->migrated)) {
          this->migrated = false;
          THPRINT("Entering thread from migration\n");
          __migrate_resume(this->my_stack); // NOTE: no stored stack
      }
      else if (this->yielded == false) {
          __clone_return(this->stored_stack);
      }
      else {
          this->yielded = false;
          __thread_restore(this->stored_stack);
      }
      __builtin_unreachable();
  }

  Thread* Thread::create(Thread* parent, long flags,
                          void* ctid, void* ptid, void* stack)
  {
    const int tid = generate_new_thread_id();
	auto* thread = new struct Thread;
	thread->init(tid, parent, stack);

	// flag for write child TID
	if (flags & CLONE_CHILD_SETTID) {
	  THPRINT("Setting ctid to %d at %p\n", tid, ctid);
	  *(int*) ctid = tid;
	}
	// flag for write parent TID
	if (flags & CLONE_PARENT_SETTID) {
	  THPRINT("Setting ptid to TID value at %p\n", ptid);
	  *(int*) ptid = tid;
	}
	if (flags & CLONE_CHILD_CLEARTID) {
	  THPRINT("Setting clear_tid to %p for tid=%d\n", ctid, tid);
	  thread->clear_tid = ctid;
	}

	return thread;
  }

  Thread* setup_main_thread(int cpu, int tid)
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
			Expects(main_thread->parent == nullptr && "Must be a detached thread");
			ThreadManager::get(cpu).main_thread = main_thread;
			return main_thread;
		}
  }

  void* get_thread_area()
  {
# ifdef ARCH_x86_64
    return (void*) x86::CPU::read_msr(IA32_FS_BASE);
# elif defined(ARCH_aarch64)
    void* thread;
    asm("mrs %0, tpidr_el0" : "=r" (thread));
    return thread;
# else
    #error "Implement me"
# endif
  }

  void set_thread_area(void* new_area) {
	  syscall_SYS_set_thread_area(new_area);
  }

  Thread* get_thread(int tid) {
	  auto& threads = ThreadManager::get().threads;
      auto it = threads.find(tid);
      if (it != threads.end()) return it->second;
	  return nullptr;
  }

  void resume(int tid)
  {
	  auto* thread = get_thread(tid);
	  if (thread != nullptr) {
	  	thread->resume();
		__builtin_unreachable();
	  }
	  THPRINT("Could not resume thread, missing: %d\n", tid);
	  Expects(thread && "Could not find thread id");
  }

  Thread* ThreadManager::detach(int tid)
  {
	  auto* thread = get_thread(tid);
	  // can't migrate missing thread
	  Expects(thread != nullptr && "Could not find given thread id");
	  // can't migrate the main thread
	  Expects(thread != ThreadManager::get().main_thread);
	  // can't migrate the thread you are in
	  auto* current = get_thread();
	  Expects(current != thread && "Can't migrate current thread");
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
      this->suspend(thread);
	  // attach this thread to the managers main thread
	  if (this->main_thread) {
		thread->attach(this->main_thread);
	  }
      // threads that are migrated from clone require special treatment
      if (thread->yielded == false) {
          thread->migrated = true;
      }
  }
  void ThreadManager::insert_thread(Thread* thread)
  {
	  threads.emplace(
			std::piecewise_construct,
			std::forward_as_tuple(thread->tid),
			std::forward_as_tuple(thread));
	  last_thread = thread;
  }
  void ThreadManager::erase_thread_safely(Thread* thread)
  {
	  Expects(thread != nullptr);
	  auto it = threads.find(thread->tid);
	  Expects(it != threads.end());
	  Expects(it->second == thread);
	  threads.erase(it);
  }
  Thread* ThreadManager::wakeup_next()
  {
	  Expects(!suspended.empty());
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
  void ThreadManager::yield_to(Thread* thread)
  {
	  // special migration-yield to next thread
	  this->next_thread = thread;
	  __thread_yield(); // NOTE: function returns!!
  }
}

// called from __thread_yield assembly, cannot return
extern "C"
void __thread_suspend_and_yield(void* stack)
{
	auto& man = kernel::ThreadManager::get();
	// don't go through the ardous yielding process when alone
	if (man.suspended.empty() && man.next_thread == nullptr) {
		THPRINT("CPU %d: Nothing to yield to. Returning... thread=%p stack=%p\n",
				SMP::cpu_id(), kernel::get_thread(), stack);
		return;
	}
	// suspend current thread (yielded)
	auto* thread = kernel::get_thread();
	thread->suspend(true, stack);

	if (man.next_thread == nullptr)
	{
		// resume some other thread
		auto* next = man.wakeup_next();
		// resume next thread
		next->resume();
	}
	else
	{
		// resume specific thread
		auto* kthread = man.next_thread;
		man.next_thread = nullptr;
        // resume the thread on this core
        man.erase_suspension(kthread);
        kthread->resume();
	}
	__builtin_unreachable();
}
