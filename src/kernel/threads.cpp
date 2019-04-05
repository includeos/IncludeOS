#include <kernel/threads.hpp>
#include <pthread.h>
#include <map>
#include <kprint>

extern "C" void __clone_exit(void* next, void* stack, long ret);

namespace kernel
{
  static int thread_counter = 1;
  static std::map<int, kernel::thread_t&> threads;
  static thread_t main_thread;

  void thread_t::init(int tid)
  {
    this->self = this;
    this->tid  = tid;
  }

  thread_t* thread_create() noexcept
  {
    const int tid = __sync_fetch_and_add(&thread_counter, 1);
    try {
      auto* thread = new thread_t;
      thread->init(tid);

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

  void thread_exit() noexcept
  {
    auto* t = get_thread();
    assert(t->tid != 0 && "Can't exit main thread");
    //kprintf("thread_exit tid=%d  RIP: %p  RSP: %p\n",
    //        t->tid, t->ret_instr, t->ret_stack);
    __clone_exit(t->ret_instr, t->ret_stack, t->tid);
    __builtin_unreachable();
  }

  void setup_main_thread() noexcept
  {
    main_thread.init(0);
    main_thread.ret_stack = nullptr;
    main_thread.ret_instr = nullptr;
  }
}

extern "C"
long syscall_SYS_sched_setscheduler(pid_t pid, int policy,
                          const struct sched_param *param)
{
  return 0;
}
