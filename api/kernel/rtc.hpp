// -*-C++-*-

#pragma once
#ifndef KERNEL_RTC_HPP
#define KERNEL_RTC_HPP

#include <cstdint>
#include <arch.hpp>

class RTC
{
public:
  using timestamp_t = uint64_t;

  /// a 64-bit nanosecond timestamp of the current time
  static timestamp_t nanos_now() {
    return __arch_system_time();
  }
  /// returns a 64-bit unix timestamp of the current time
  static timestamp_t now() {
    return __arch_wall_clock().tv_sec;
  }

  /// returns a 64-bit unix timestamp for when the OS was booted
  static timestamp_t boot_timestamp() {
    return booted_at;
  }

  /// returns system uptime in seconds
  static timestamp_t time_since_boot() {
    return now() - boot_timestamp();
  }

  /// start time auto-calibration process
  static void init();

private:
  static timestamp_t booted_at;
};

#endif
