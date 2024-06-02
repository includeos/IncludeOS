#include <arch/x86/cpu.hpp>
#include <kernel/threads.hpp>
#include <os.hpp>
#include <common>
#include <kprint>

extern "C" {
  long syscall_SYS_set_thread_area(void* u_info);
  void __clone_return(void* stack);
}

extern "C"
long clone_helper(
        void* callback,
        void* stack,
        unsigned long flags,
        void* userdata,
        void* ptid,
        void* newtls,
        void* ctid,
        void* old_stack)
{
    // NOTE: using printf is completely forbidden in this function
    auto* parent = kernel::get_thread();

    auto* thread = kernel::Thread::create(parent, flags, ctid, ptid, stack);

//#define VERBOSE_CLONE_FUNCTION
#ifdef VERBOSE_CLONE_FUNCTION
    extern int __thread_list_lock;
    kprintf("clone syscall creating thread %d\n", thread->tid);
    kprintf("-> callback: %p\n", callback);
    kprintf("-> stack:  %p\n",  stack);
    kprintf("-> flags:  %#lx\n", flags);
    kprintf("-> argument: %p\n", userdata);
    kprintf("-> ptid:   %p\n", ptid);
    kprintf("-> ctid:   %p vs %p\n", ctid, nullptr);
    kprintf("-> tls:    %p vs %p\n", newtls, *(void**) newtls);
    kprintf("-> old thread: %p\n", parent);
    kprintf("-> old stack: %p\n", old_stack);
    kprintf("-> old tls: %p (%p)\n", kernel::get_thread_area(), parent->my_tls);
#endif

    // write tid on the top of the old stack (for parent)
    *(uintptr_t*) old_stack = thread->tid;

	// set TLS location (and set self)
    thread->set_tls(newtls);

	auto& tman = kernel::ThreadManager::get();
	if (tman.on_new_thread != nullptr) {
		// potentially get child stolen by migration callback
		thread = tman.on_new_thread(tman, thread);
	}

	if (thread) {
		// if we still have the thread, insert it now
		tman.insert_thread(thread);

        THPRINT("Suspending parent thread tid=%d tls=%p stack=%p and entering %d\n",
                parent->tid, parent->my_tls, old_stack, thread->tid);
		// suspend parent thread (not yielded)
		parent->suspend(false, old_stack);
		// continue on child
		kernel::set_thread_area(thread->my_tls);
		return thread->tid;
	}
    THPRINT("Returning to parent thread stack=%p\n", old_stack);
	// continue with parent
	__clone_return(old_stack);
	__builtin_unreachable();
}
