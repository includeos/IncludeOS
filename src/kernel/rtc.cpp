#include <kernel/rtc.hpp>

#include <kernel/os.hpp>
#include <kernel/timers.hpp>
#include <hw/cpu.hpp>
#include <hw/cmos.hpp>
#include <hertz>

static int64_t  current_time  = 0;
static uint64_t current_ticks = 0;

using namespace std::chrono;

void RTC::init()
{
  // Initialize CMOS
  cmos::init();

  // set current timestamp and ticks
  current_time  = cmos::now().to_epoch();
  current_ticks = hw::CPU::rdtsc();

  // every minute recalibrate
  Timers::periodic(seconds(60), seconds(60),
  [] (uint32_t) {
    current_time  = cmos::now().to_epoch();
    current_ticks = hw::CPU::rdtsc();
  });
}

RTC::timestamp_t RTC::now()
{
  auto ticks = hw::CPU::rdtsc() - current_ticks;
  auto diff  = ticks / Hz(MHz(OS::cpu_freq())).count();

  return current_time + diff;
}
