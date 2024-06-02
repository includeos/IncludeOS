#include "stub.hpp"
#include <kernel/threads.hpp>
#include <os.hpp>
#include <smp>

#define FUTEX_WAIT 0
#define FUTEX_WAKE 1
#define FUTEX_FD 2
#define FUTEX_REQUEUE 3
#define FUTEX_CMP_REQUEUE 4
#define FUTEX_WAKE_OP 5
#define FUTEX_LOCK_PI 6
#define FUTEX_UNLOCK_PI 7
#define FUTEX_TRYLOCK_PI 8
#define FUTEX_WAIT_BITSET 9
#define FUTEX_PRIVATE 128
#define FUTEX_CLOCK_REALTIME 256

#ifdef FUTEX_WAITING_LISTS
//#define VERBOSE_FUTEX_SYSCALL
#include <unordered_set>
struct Wordlist
{
	Wordlist() {
		cpus.reserve(256);
	}
	volatile int* uaddr = nullptr;
	std::unordered_set<int> cpus;
};
static std::array<Wordlist, 16> wordlist;
static smp_spinlock wlspinner;

inline Wordlist& wait_on(volatile int* uaddr, int value)
{
	wlspinner.lock();
	for (auto& word : wordlist) {
		if (word.uaddr == uaddr) {
			auto p = word.cpus.insert(SMP::cpu_id());
			Expects(p.second && "Inserted");
			wlspinner.unlock();
			return word;
		}
	}
	for (auto& word : wordlist) {
		if (word.uaddr == nullptr) {
			word.uaddr = uaddr;
			word.cpus.insert(SMP::cpu_id());
			wlspinner.unlock();
			return word;
		}
	}
	wlspinner.unlock();
	Expects(false && "No more futex word space");
	__builtin_unreachable();
}
inline void unwait_on(Wordlist& word, volatile int* uaddr)
{
	wlspinner.lock();
	Expects(word.uaddr == uaddr);
	word.cpus.erase(SMP::cpu_id());
	if (word.cpus.empty()) {
#ifdef VERBOSE_FUTEX_SYSCALL
		SMP::global_lock();
		kprintf("CPU %d freed word %p\n", SMP::cpu_id(), word.uaddr);
		SMP::global_unlock();
#endif
		word.uaddr = nullptr;
	}
	wlspinner.unlock();
}
inline int wake_word(volatile int* uaddr)
{
	wlspinner.lock();
	for (auto& word : wordlist)
	{
		if (word.uaddr == uaddr) {
			std::array<int16_t, 256> buffer;
			int dreamers = 0;
			for (int cpu : word.cpus) {
				// send spurious wakeup to each waiter
				if (cpu != SMP::cpu_id()) {
#ifdef VERBOSE_FUTEX_SYSCALL
					SMP::global_lock();
					kprintf("%d: Waking up CPU %d on word %p\n", 
							SMP::cpu_id(), cpu, uaddr);
					SMP::global_unlock();
#endif
					buffer[dreamers++] = cpu;
				}
			}
			wlspinner.unlock();
			for (int i = 0; i < dreamers; i++) {
				SMP::wake(buffer[i]);
			}
			return dreamers;
		}
	}
	wlspinner.unlock();
	return 0;
}
#endif

static long sys_futex(volatile int *uaddr, int futex_op, int val,
                      const struct timespec* /* timeout */, int /*val3*/)
{
	switch (futex_op & 0xF) {
		case FUTEX_WAIT:
			// fast-path
			if (*uaddr != val) return -EAGAIN;
#ifdef VERBOSE_FUTEX_SYSCALL
			SMP::global_lock();
			kprintf("CPU %d waiting on word %p\n", SMP::cpu_id(), uaddr);
			SMP::global_unlock();
#endif
			// cooperative path
			if (SMP::cpu_count() == 1) {
				// we have to yield here because of cooperative threads
				while (*uaddr == val) __thread_yield();
				return 0;
			}
			else // multiprocessing-path
			{
				while (*uaddr == val) asm("pause");
/*
				auto& word = wait_on(uaddr, val);
				while (*uaddr == val) asm("hlt");
#ifdef VERBOSE_FUTEX_SYSCALL
				SMP::global_lock();
				kprintf("CPU %d done waiting on word %p\n", SMP::cpu_id(), uaddr);
				SMP::global_unlock();
#endif
				unwait_on(word, uaddr);
*/
				return 0;
			}
		case FUTEX_WAKE:
			//return wake_word(uaddr);
			return 0;
		default:
			return -ENOSYS;
	}
}

extern "C"
int syscall_SYS_futex(volatile int *uaddr, int futex_op, int val,
                      const struct timespec *timeout, int val3)
{
  return strace(sys_futex, "futex", uaddr, futex_op, val, timeout, val3);
}
