#include "stub.hpp"
#include <errno.h>
#include <kprint>

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

extern void print_backtrace();

static int sys_futex(int *uaddr, int /*futex_op*/, int val,
                      const struct timespec *timeout, int /*val3*/)
{

  if (*uaddr != val){
    return EAGAIN;
  } else {
    *uaddr = 0;
  }

  if (timeout == nullptr){
    kprintf("No timeout\n");
  }

  return 0;
}

extern "C"
int syscall_SYS_futex(int *uaddr, int futex_op, int val,
                      const struct timespec *timeout, int val3)
{
  return stubtrace(sys_futex, "futex", uaddr, futex_op, val, timeout, val3);
}
