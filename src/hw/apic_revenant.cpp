#include <cstdio>
#include <cstdint>
//#include <atomic>

extern "C"
void revenant_main(int);
extern int boot_counter;

typedef volatile int spinlock_t;
static spinlock_t glock = 0;

void lock(spinlock_t& lock) {
  while (__sync_lock_test_and_set(&lock, 1)) {
    while (lock);
  }
}

void unlock(spinlock_t& lock) {
  __sync_synchronize(); // barrier
  lock = 0;
}

void revenant_main(int cpu)
{
  // we can use shared memory here because the
  // bootstrap CPU is waiting on revenants to start
  // we do, however, need to synchronize in between CPUs
  lock(glock);
  printf("Revenant %u started\n", cpu);
  unlock(glock);
  // signal that the revenant is started
  asm volatile("lock incl %0" : "=m"(boot_counter));
  
  // do something useful
  asm volatile("cli; hlt;");
}
