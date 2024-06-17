#include "common.hpp"
#include <time.h>
#include <timers>
using namespace std::chrono;

static void nanosleep(nanoseconds nanos)
{
  bool ticked = false;

  Timers::oneshot(nanos,
  [&ticked] (int) {
    ticked = true;
  });

  while (ticked == false) {
    os::block();
  }
}

static long sys_nanosleep(const struct timespec* req, struct timespec */*rem*/)
{
  if (req == nullptr) return -EINVAL;
  auto nanos = nanoseconds(req->tv_sec * 1'000'000'000ull + req->tv_nsec);
  nanosleep(nanos);
  return 0;
}

extern "C"
long syscall_SYS_nanosleep(const struct timespec *req, struct timespec *rem)
{
  return strace(sys_nanosleep, "nanosleep", req, rem);
}

extern "C"
long syscall_SYS_clock_nanosleep(clockid_t, int,
        const struct timespec *req, struct timespec *rem)
{
  return strace(sys_nanosleep, "clock_nanosleep", req, rem);
}

extern "C"
long syscall_SYS_clock_nanosleep_time64(clockid_t, int,
        const struct timespec *req, struct timespec *rem)
{
  return strace(sys_nanosleep, "clock_nanosleep_time64", req, rem);
}
