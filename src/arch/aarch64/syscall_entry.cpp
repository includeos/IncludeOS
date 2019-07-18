#include "cpu.h"
#include <kernel/threads.hpp>

extern "C"
pthread_t syscall_clone(
        unsigned long flags,
        void *stack,
        int *ptid,
        unsigned long newtls,
        int *ctid,
        // needed to suspend this thread
        void* next_instr,
        void* old_stack)
{
    auto* parent = kernel::get_thread();
    auto* thread = kernel::thread_create(parent, flags, ctid, stack);

    // suspend parent thread
    parent->suspend(next_instr, old_stack);
    // activate new TLS location
    thread->activate(newtls);
    return thread->tid;
}

extern "C"
long syscall_SYS_set_thread_area(struct user_desc *u_info)
{
  set_tpidr(u_info);
  return 0;
}
