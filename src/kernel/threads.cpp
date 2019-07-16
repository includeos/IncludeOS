#include <kernel/threads.hpp>
#include <arch/x86/cpu.hpp>
#include <kprint>
#include <cassert>
#include <deque>
#include <map>
#include <pthread.h>

extern "C" {
  void __thread_yield();
  void __thread_restore(void* nexti, void* stack, long tid);
  void __clone_return(void* nexti, void* stack, long tid);
  long syscall_SYS_set_thread_area(void* u_info);
}

struct libc_internal {
  void* self;
  void* dtv;
  kernel::thread_t* kthread;
};

namespace kernel
{
  static int64_t thread_counter = 1;
  static std::map<int64_t, kernel::thread_t&> threads;
  static std::deque<thread_t*> suspended;
  static thread_t main_thread;

  void thread_t::init(int tid)
  {
    this->self = this;
    this->tid  = tid;
  }

  void thread_t::libc_store_this()
  {
    auto* s = (libc_internal*) this->my_tls;
    s->kthread = this;
  }
  void thread_t::store_return(void* ret_instr, void* ret_stack)
  {
    THPRINT("thread %ld storing return point %p with stack %p\n",
            this->tid, ret_instr, ret_stack);
    this->stored_nexti = ret_instr;
    this->stored_stack = ret_stack;
  }
  void thread_t::activate(void* newtls)
  {
    this->my_tls = newtls;
    // store ourselves in the guarded libc structure
    this->libc_store_this();
    syscall_SYS_set_thread_area(this->my_tls);
  }

  void thread_t::yield()
  {
      this->yielded = true;
      // add to suspended (NB: can throw)
      suspended.push_back(this);
      // resume a waiting thread
      auto* next = suspended.front();
      suspended.pop_front();
      // resume next thread
      next->resume(this->tid);
  }

  void thread_t::exit()
  {
    assert(this->parent != nullptr);
    // detach children
    for (auto* child : this->children) {
        child->parent = &main_thread;
    }
    // resume parent with this as child
    this->parent->resume(this->tid);
  }

  void thread_t::resume(long ctid)
  {
      THPRINT("Returning to tid=%ld tls=%p nexti=%p stack=%p\n",
            this->tid, this->my_tls, this->stored_nexti, this->stored_stack);
      syscall_SYS_set_thread_area(this->my_tls);
      // NOTE: the RAX return value here is CHILD thread id, not this
      if (yielded == false) {
          __clone_return(this->stored_nexti, this->stored_stack, ctid);
      }
      else {
          __thread_restore(this->stored_nexti, this->stored_stack, ctid);
      }
      __builtin_unreachable();
  }

  thread_t* thread_create(thread_t* parent) noexcept
  {
    const int tid = __sync_fetch_and_add(&thread_counter, 1);
    try {
      auto* thread = new thread_t;
      thread->init(tid);
      thread->parent = parent;

      threads.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(tid),
            std::forward_as_tuple(*thread));
      return thread;
    }
    catch (...) {
      return nullptr;
    }
  }

  void setup_main_thread() noexcept
  {
    main_thread.init(0);
    // allow exiting in main thread
    main_thread.my_tls = get_thread_area();
    main_thread.libc_store_this();
  }

  void* get_thread_area()
  {
# ifdef ARCH_x86_64
    return (void*) x86::CPU::read_msr(IA32_FS_BASE);
# else
    #error "Implement me"
# endif
  }
}

extern "C"
void __thread_self_store(void* next_instr, void* stack)
{
  auto* thread = kernel::get_thread();
  thread->store_return(next_instr, stack);
  thread->yield();
}

extern "C"
long syscall_SYS_sched_setscheduler(pid_t /*pid*/, int /*policy*/,
                          const struct sched_param* /*param*/)
{
  return 0;
}
