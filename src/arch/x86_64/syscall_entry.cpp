
#include <arch/x86/cpu.hpp>
#include <kernel/threads.hpp>
#include <os.hpp>
#include <common>
#include <kprint>
#include <errno.h>

extern "C" {
  long syscall_SYS_set_thread_area(void* u_info);
  void __clone_return(void* stack);
}

#define ARCH_SET_GS 0x1001
#define ARCH_SET_FS 0x1002
#define ARCH_GET_FS 0x1003
#define ARCH_GET_GS 0x1004

#ifdef __x86_64__
static long sys_prctl(int code, uintptr_t ptr)
{
  return -ENOSYS;
  switch(code){
  case ARCH_SET_GS:
    //kprintf("<arch_prctl> set_gs to %#lx\n", ptr);
    if (UNLIKELY(!ptr)) return -EINVAL;
    x86::CPU::set_gs((void*)ptr);
    break;
  case ARCH_SET_FS:
    //kprintf("<arch_prctl> set_fs to %#lx\n", ptr);
    if (UNLIKELY(!ptr)) return -EINVAL;
    x86::CPU::set_fs((void*)ptr);
    break;
  case ARCH_GET_GS:
    os::panic("<arch_prctl> GET_GS called!\n");
  case ARCH_GET_FS:
    os::panic("<arch_prctl> GET_FS called!\n");
  }
  return -EINVAL;
}
#endif

#include <kernel/elf.hpp>
static void print_symbol(const void* addr)
{
  char buffer[8192];
  auto symb = Elf::safe_resolve_symbol(addr, buffer, sizeof(buffer));
  kprintf("0x%lx + 0x%.3x: %s\n",
          symb.addr, symb.offset, symb.name);
}

extern "C"
pthread_t syscall_clone(void* next_instr,
                    unsigned long flags, void* stack,
                    void* ptid, void* ctid, void* newtls,
                    void* old_stack, void* callback)
{
    auto* parent = kernel::get_thread();

    auto* thread = kernel::thread_create(parent, flags, ctid, ptid, stack);
#ifdef VERBOSE_CLONE_SYSCALL
    kprintf("clone syscall creating thread %ld\n", thread->tid);
    kprintf("-> nexti:  "); print_symbol(next_instr);
    kprintf("-> flags:  %#lx\n", flags);
    kprintf("-> stack:  %p\n",  stack);
    kprintf("-> parent: %p\n", ptid);
    kprintf("-> child:  %p\n", ctid);
    kprintf("-> tls:    %p\n", newtls);
    kprintf("-> old stack: %p\n", old_stack);
    kprintf("-> old tls: %p\n", kernel::get_thread_area());
    kprintf("-> callback: "); print_symbol(callback);
#endif

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
uintptr_t syscall_entry(long n, long a1, long a2, long a3, long a4, long a5)
{
  switch(n) {
  case 56: // clone
    assert(0 && "Clone needs to be implemented in assembly");
  case 57: // fork
    return -ENOSYS;
  case 58: // vfork
    return -ENOSYS;
  case 158: // arch_prctl
    sys_prctl(a1, a2);
    break;
  default:
    kprintf("<syscall entry> no %lu (a1=%#lx a2=%#lx a3=%#lx a4=%#lx a5=%#lx) \n",
            n, a1, a2, a3, a4, a5);
  }
  return 0;
}

extern "C"
long syscall_SYS_set_thread_area(void* u_info)
{
  //kprintf("<SYS_set_thread_area> set to %p\n", u_info);
  if (UNLIKELY(!u_info)) return -EINVAL;
#ifdef __x86_64__
#	ifdef PLATFORM_x86_solo5
		solo5_set_tls_base((uintptr_t) u_info);
#	else
  		x86::CPU::set_fs(u_info);
#	endif
#else
  x86::CPU::set_gs(u_info);
#endif
  return 0;
}
