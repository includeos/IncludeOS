#include <kernel/rtc.hpp>

#include <kernel/os.hpp>
#include <kernel/timers.hpp>
#include <hw/cmos.hpp>
#include <hertz>

using timestamp_t = RTC::timestamp_t;
timestamp_t RTC::booted_at;
timestamp_t RTC::current_time;
uint64_t    RTC::current_ticks;

using namespace std::chrono;

void RTC::init()
{
  // Initialize CMOS
  cmos::init();

  // set current timestamp and ticks
  current_time  = cmos::now().to_epoch();
  current_ticks = OS::cycles_since_boot();

  // set boot timestamp
  booted_at = current_time;

  INFO("RTC", "Enabling regular clock sync with CMOS");
  // every minute recalibrate
  Timers::periodic(seconds(60), seconds(60),
  [] (Timers::id_t) {
    current_time  = cmos::now().to_epoch();
    current_ticks = OS::cycles_since_boot();
  });
}

timestamp_t RTC::now()
{
  auto ticks = OS::cycles_since_boot() - current_ticks;
  auto diff  = ticks / Hz(MHz(OS::cpu_freq())).count();

  return current_time + diff;
}
