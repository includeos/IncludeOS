#include "cpu.h"
#include <kernel/threads.hpp>

extern "C" {
  long syscall_SYS_set_thread_area(void* u_info);
  void __clone_return(void* stack);
}

extern "C"
long syscall_clone(
        unsigned long flags,
        void *stack,
        int *ptid,
        void *newtls,
        int *ctid,
        // needed to suspend this thread
        void* next_instr,
        void* old_stack)
{
    auto* parent = kernel::get_thread();
    auto* thread = kernel::thread_create(parent, flags, ctid, ptid, stack);

    // set TLS location (and set self)
    thread->set_tls(newtls);

	auto& tman = kernel::ThreadManager::get();
	if (tman.on_new_thread != nullptr) {
		// push 8 values onto new stack, as the old stack will get
		// used immediately by the returning thread
		constexpr int STV = 8;
		for (int i = 0; i < STV; i++) {
			thread->stack_push(*((uintptr_t*) old_stack + STV + 1 - i));
		}
		// potentially get child stolen by migration callback
		thread = tman.on_new_thread(tman, thread);
	}

	if (thread) {
		// suspend parent thread (not yielded)
		parent->suspend(false, old_stack);
		// continue on child
		kernel::set_thread_area(thread->my_tls);
		return thread->tid;
	}
	// continue with parent
	__clone_return(old_stack);
	__builtin_unreachable();
}

extern "C"
long syscall_SYS_set_thread_area(void* u_info)
{
  set_tpidr(u_info);
  return 0;
}
