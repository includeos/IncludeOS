#include <kernel.hpp>
#include <kernel/events.hpp>
#include <kernel/rng.hpp>
#include <kernel/service.hpp>
#include <kernel/timers.hpp>
#include <system_log>
#include <sys/random.h>
#include <sys/time.h>
#include <unistd.h>
#ifndef PORTABLE_USERSPACE
#include <sched.h>
#include "epoll_evloop.hpp"
#endif

void os::event_loop()
{
  Events::get().process_events();
  do
  {
    Timers::timers_handler();
    Events::get().process_events();
#ifndef PORTABLE_USERSPACE
    if (kernel::is_running()) linux::epoll_wait_events();
#endif
    Events::get().process_events();
  }
  while (kernel::is_running());
  // call on shutdown procedure
  Service::stop();
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

// timer system
static void begin_timer(std::chrono::nanoseconds) {}
static void stop_timers() {}

void kernel::start(const char* cmdline)
{
  kernel::state().libc_initialized = true;
  // setup timer system
  Timers::init(begin_timer, stop_timers);
  Timers::ready();
  // seed RNG with entropy
  char entropy[2048];
  ssize_t rngres = getrandom(entropy, sizeof(entropy), 0);
  assert(rngres == sizeof(entropy));
  rng_absorb(entropy, sizeof(entropy));
  // fake CPU frequency
  kernel::state().cmdline = cmdline;
  kernel::state().cpu_khz = decltype(os::cpu_freq()) {3000000ul};
}

// stdout
void kernel::default_stdout(const char* text, size_t len)
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

multiboot_info_t* kernel::bootinfo() {
  return nullptr;
}

void kernel::init_heap(uintptr_t, uintptr_t) noexcept {

}
bool kernel::heap_ready() { return true; }
bool os::mem::heap_ready() { return true; }

size_t kernel::heap_usage() noexcept {
  return 0;
}
size_t kernel::heap_avail() noexcept {
  return 0xFFFFFFFF;
}
uintptr_t kernel::heap_end() noexcept {
  return 0x7FFFFFFF;
}

#include <memory>
namespace os::mem
{
  uintptr_t virt_to_phys(uintptr_t linear) {
    return linear;
  }
  size_t min_psize() {
    return 4096;
  }
}
