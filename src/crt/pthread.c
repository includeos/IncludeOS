#include <stdio.h>
#define UNLOCKED 0
#define LOCKED   1

extern void panic(const char* why);
typedef int spinlock_t;

void yield()
{
  printf("yield() called, but not implemented yet!");
}

static int compare_and_swap(volatile spinlock_t* addr)
{
  const int expected = UNLOCKED;
  const int newval   = LOCKED;
  return __sync_val_compare_and_swap(addr, expected, newval);
}


int pthread_mutex_init(volatile spinlock_t* lock)
{
  *lock = UNLOCKED;
  return 0;
}
int pthread_mutex_destroy(volatile spinlock_t* lock)
{
  (void) lock;
  return 0;
}

int pthread_mutex_lock(volatile spinlock_t* lock)
{
  while (!compare_and_swap(lock))
      yield();
  return 0;
}

int pthread_mutex_unlock(volatile spinlock_t* lock)
{
  *lock = UNLOCKED;
  return 0;
}
