#include <kernel/os.hpp>
#include <kernel/events.hpp>
#include <kernel/service.hpp>
#include <kernel/timers.hpp>
#include <system_log>
#include <sys/time.h>
#include <malloc.h> // mallinfo()
#include <sched.h>
extern bool __libc_initialized;

void OS::event_loop()
{
  Events::get().process_events();
  do
  {
    Timers::timers_handler();
    // FIXME: wait on first one
    extern void wait_tap_devices();
    wait_tap_devices();
    Events::get().process_events();
  }
  while (OS::is_running());
  // call on shutdown procedure
  Service::stop();
}

uintptr_t OS::heap_begin() noexcept {
  return 0;
}
uintptr_t OS::heap_end() noexcept {
  return 0;
}
uintptr_t OS::heap_usage() noexcept {
  auto info = mallinfo();
  return info.arena + info.hblkhd;
}
uintptr_t OS::heap_max() noexcept
{
  return (uintptr_t) -1;
}

bool OS::is_panicking() noexcept
{
  return false;
}

#include <kernel/rtc.hpp>
#include <time.h>
RTC::timestamp_t RTC::booted_at = time(0);

#include <smp>
int SMP::cpu_id() noexcept {
  return 0;
}
void SMP::global_lock() noexcept {}
void SMP::global_unlock() noexcept {}
void SMP::add_task(SMP::task_func, int) {}
void SMP::signal(int) {}

#include <service>
__attribute__((weak)) void Service::ready() {}
__attribute__((weak)) void Service::stop() {}
extern const char* service_name__;
extern const char* service_binary_name__;
const char* Service::name() {
  return service_name__;
}
const char* Service::binary_name() {
  return service_binary_name__;
}

// timer system
#include <signal.h>
#include <unistd.h>
static timer_t timer_id;
extern "C" void alarm_handler(int sig)
{
  (void) sig;
}
static void begin_timer(std::chrono::nanoseconds usec)
{
  using namespace std::chrono;
  auto secs = duration_cast<seconds> (usec);

  struct itimerspec it;
  it.it_value.tv_sec  = secs.count();
  it.it_value.tv_nsec = usec.count() - secs.count() * 1000000000ull;
  timer_settime(timer_id, 0, &it, nullptr);
}
static void stop_timers() {}

void OS::start(const char* cmdline)
{
  __libc_initialized = true;
  // setup Linux timer (with signal handler)
  struct sigevent sev;
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo  = SIGALRM;
  timer_create(CLOCK_BOOTTIME, &sev, &timer_id);
  signal(SIGALRM, alarm_handler);
  // setup timer system
  Timers::init(begin_timer, stop_timers);
  Timers::ready();
  // fake CPU frequency
  using namespace std::chrono;
  OS::cpu_khz_ = decltype(OS::cpu_freq()) {3000000ul};
  OS::cmdline = cmdline;
}

// stdout
void OS::default_stdout(const char* text, size_t len)
{
  ssize_t bytes = write(STDOUT_FILENO, text, len);
  assert(bytes == (ssize_t) len);
}

// system_log has no place on Linux because stdout goes --> pipe
void SystemLog::initialize() {}

#ifdef __MACH__
#include <stdlib.h>
#include <stddef.h>
#include <gsl/gsl_assert>
void* memalign(size_t alignment, size_t size) {
  void* ptr {nullptr};
  int res = posix_memalign(&ptr, alignment, size);
  Ensures(res == 0);
  return ptr;
}
void* aligned_alloc(size_t alignment, size_t size) {
  return memalign(alignment, size);
}
#endif
