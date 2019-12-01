#include <kernel/threads.hpp>
#include <arch/x86/cpu.hpp>
#include <kprint>
#include <cassert>
#include <deque>
#include <map>
#include <pthread.h>

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
  static int64_t thread_counter = 1;
  static std::map<int64_t, kernel::Thread*> threads;
  static std::deque<Thread*> suspended;
  static Thread main_thread;

  static void erase_suspension(Thread* t)
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

  int64_t get_last_thread_id() noexcept {
	  return thread_counter-1;
  }

  void Thread::init(int tid)
  {
    this->self = this;
    this->tid  = tid;
  }

  void Thread::libc_store_this()
  {
    auto* s = (libc_internal*) this->my_tls;
    s->kthread = this;
  }
  void Thread::store_return(void* ret_instr, void* ret_stack)
  {
    THPRINT("Thread %ld storing return point %p with stack %p\n",
            this->tid, ret_instr, ret_stack);
    this->stored_nexti = ret_instr;
    this->stored_stack = ret_stack;
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
      this->store_return(ret_instr, ret_stack);
      // add to suspended (NB: can throw)
      suspended.push_back(this);
  }
  void Thread::yield()
  {
      // resume a waiting Thread
      assert(!suspended.empty());
      auto* next = suspended.front();
      suspended.pop_front();
      // resume next Thread
      this->yielded = true;
      next->resume();
  }

  void Thread::exit()
  {
    const bool exiting_myself = (get_thread() == this);
    assert(this->parent != nullptr);
    // detach children
    for (auto* child : this->children) {
        child->parent = &main_thread;
    }
    // remove myself from parent
    auto& pcvec = this->parent->children;
    for (auto it = pcvec.begin(); it != pcvec.end(); ++it) {
        if (*it == this) {
            pcvec.erase(it); break;
        }
    }
    // temporary copy of parent Thread pointer
    auto* next = this->parent;
    // CLONE_CHILD_CLEARTID: set userspace TID value to zero
    if (this->clear_tid) {
        THPRINT("Clearing child value at %p\n", this->clear_tid);
        *(pthread_t*) this->clear_tid = 0;
    }
    // delete this Thread
    auto it = threads.find(this->tid);
    assert(it != threads.end());
    assert(it->second == this);
    threads.erase(it);
    // free Thread resources
    delete this;
    // resume parent Thread
    if (exiting_myself)
    {
        erase_suspension(next);
        next->resume();
    }
  }

  void Thread::resume()
  {
      THPRINT("Returning to tid=%ld tls=%p nexti=%p stack=%p\n",
            this->tid, this->my_tls, this->stored_nexti, this->stored_stack);
      // NOTE: the RAX return value here is CHILD Thread id, not this
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
    const int tid = __sync_fetch_and_add(&thread_counter, 1);
    try {
      auto* Thread = new struct Thread;
      Thread->init(tid);
      Thread->parent = parent;
      Thread->parent->children.push_back(Thread);
      Thread->my_stack = stack;

      // flag for write child TID
      if (flags & CLONE_CHILD_SETTID) {
          *(pid_t*) ctid = Thread->tid;
      }
      if (flags & CLONE_CHILD_CLEARTID) {
          Thread->clear_tid = ctid;
      }

      threads.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(tid),
            std::forward_as_tuple(Thread));
      return Thread;
    }
    catch (...) {
      return nullptr;
    }
  }

  void setup_main_thread() noexcept
  {
      int stack_value;
      main_thread.init(0);
      main_thread.my_stack = (void*) &stack_value;
      // allow exiting in main Thread
      main_thread.activate(get_thread_area());
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

  Thread* get_thread(int64_t tid) {
      auto it = threads.find(tid);
      if (it == threads.end()) return nullptr;
      return it->second;
  }

  void resume(int64_t tid)
  {
	  auto* Thread = get_thread(tid);
	  assert(Thread);
	  Thread->resume();
  }
}

extern "C"
void __thread_suspend_and_yield(void* next_instr, void* stack)
{
    // don't go through the ardous yielding process when alone
    if (kernel::suspended.empty()) return;
    // suspend current Thread
    auto* Thread = kernel::get_thread();
    Thread->suspend(next_instr, stack);
    // resume some other Thread
    Thread->yield();
}

extern "C"
long syscall_SYS_sched_setscheduler(pid_t /*pid*/, int /*policy*/,
                          const struct sched_param* /*param*/)
{
  return 0;
}
