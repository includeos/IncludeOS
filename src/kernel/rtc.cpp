#include <kernel/rtc.hpp>

RTC::timestamp_t RTC::booted_at;

void RTC::init()
{
  // set boot timestamp
  booted_at = now();
}
