#include "common.hpp"
#include <errno.h>

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

extern "C" void panic(char*);
extern void print_backtrace();

extern "C"
int syscall_SYS_futex(int *uaddr, int futex_op, int val,
                      const struct timespec *timeout, int val3)
{
  STRACE("syscall futex: uaddr=%p futex_op=0x%x val=%i timeout=%p,  val3: %i\n",
         uaddr, futex_op, val, timeout, val3);

  //print_backtrace();
  if (*uaddr != val){
    return EAGAIN;
  } else {
    *uaddr = 0;
  }

  if (timeout == nullptr){
    kprintf("No timeout\n");
  }

  kprintf("Futex ok\n");
  return 0;
}
