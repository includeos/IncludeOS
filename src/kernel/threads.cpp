#include <kernel/threads.hpp>
#include <arch/x86/cpu.hpp>
#include <pthread.h>
#include <map>
#include <kprint>

extern "C" {
  void __clone_exit(void* next, void* stack, long ret);
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
    //kprintf("thread_exit tid=%ld  RIP: %p  RSP: %p  TLS: %#lx\n",
    //        t->tid, t->ret_instr, t->ret_stack, t->ret_tls);

    syscall_SYS_set_thread_area((void*) t->ret_tls);
    __clone_exit(t->ret_instr, t->ret_stack, t->tid);
    __builtin_unreachable();
  }

  void setup_main_thread() noexcept
  {
    main_thread.init(0);
    main_thread.ret_stack = nullptr;
    main_thread.ret_instr = nullptr;
    main_thread.ret_tls = 0;
    // allow exiting in main thread
    uint64_t tls = x86::CPU::read_msr(IA32_FS_BASE);
    auto* s = (libc_internal*) tls;
    s->kthread = &main_thread;
  }
}

extern "C"
long syscall_SYS_sched_setscheduler(pid_t /*pid*/, int /*policy*/,
                          const struct sched_param* /*param*/)
{
  return 0;
}
