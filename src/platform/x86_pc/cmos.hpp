// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015-2016 Oslo and Akershus University College of Applied Sciences
// and Alfred Bratterud
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#ifndef HW_CMOS_HPP
#define HW_CMOS_HPP

#include <hw/ioport.hpp>

namespace x86
{
  /** Functions / classes for x86 CMOS / RTC interaction */
  class CMOS {
  public:
    using port_t = const uint8_t;
    using reg_t =  const uint8_t;
    using bit_t =  const uint8_t;


    // CMOS layout source:
    // http://bos.asmhackers.net/docs/timer/docs/cmos.pdf
    static const port_t select = 0x70;
    static const port_t data = 0x71;

    static const bit_t no_nmi = 0x80;

    // CMOS Time (RTC) registers
    static const reg_t r_sec = 0x0;
    static const reg_t r_min = 0x2;
    static const reg_t r_hrs = 0x4;
    static const reg_t r_dow = 0x6;
    static const reg_t r_day = 0x7;
    static const reg_t r_month = 0x8;
    static const reg_t r_year = 0x9;
    //static const reg_t r_cent = 0x48;

    // RTC Alarm registers
    static const reg_t r_alarm_sec = 0x1;
    static const reg_t r_alarm_min = 0x3;
    static const reg_t r_alarm_hrs = 0x5;
    static const reg_t r_alarm_day = 0x5;

    // Status reg. A
    static const reg_t r_status_a = 0xa;
    static const bit_t a_time_update_in_progress = 0x80;

    // Status reg. B
    static const reg_t r_status_b = 0xb;
    static const bit_t b_update_cycle_normal = 0x80;
    static const bit_t b_periodic_int_enabled = 0x40;
    static const bit_t b_alarm_int_enabled = 0x20;
    static const bit_t b_update_ended_int = 0x10;
    static const bit_t b_square_wave_enabled = 0x8;
    static const bit_t b_binary_mode = 0x4;
    static const bit_t b_24_hr_clock = 0x2;
    static const bit_t b_daylight_savings_enabled = 0x1;

    // Status reg. C ... (incomplete)
    static const reg_t r_status_c = 0xc;
    static const reg_t r_status_d = 0xd;

    // Memory registers
    static const reg_t r_lowmem_lo = 0x15;
    static const reg_t r_lowmem_hi = 0x16;
    static const reg_t r_himem_lo = 0x17;
    static const reg_t r_himem_hi = 0x18;
    static const reg_t r_memsize_lo = 0x30;
    static const reg_t r_memsize_hi = 0x31;

    /** Get the contents of a CMOS register */
    static uint8_t get(reg_t reg) {
      hw::outb(select, reg | no_nmi);
      return hw::inb(data);
    }

    /** Set the contents of a CMOS register */
    static void set(reg_t reg, uint8_t bits) {
      debug("CMOS setting bits 0x%x in reg. 0x%x \n", bits, reg);
      hw::outb(select, reg | no_nmi);
      hw::outb(data, bits);
    }

    /** Check if the CMOS time registers are being updated */
    static bool update_in_progress(){
      return get(r_status_a) & a_time_update_in_progress;
    }

    /** Initialize CMOS with 24 hour format UTC */
    static void init();

    static uint8_t bcd_to_binary(uint8_t value) {
      return (value & 0x0F) + ((value / 16) * 10);
    }

    union mem_t {
      uint16_t total;
      struct {
        uint8_t lo;
        uint8_t hi;
      };
    };

    struct memory_t {
      mem_t base;
      mem_t extended;
      mem_t actual_extended;
    };

    static memory_t meminfo(){
      memory_t mem {{0}, {0}, {0}};
      mem.base.hi = get(r_lowmem_hi);
      mem.base.lo = get(r_lowmem_lo);
      mem.extended.hi = get(r_himem_hi);
      mem.extended.lo = get(r_himem_lo);
      mem.actual_extended.hi = get(r_memsize_hi);
      mem.actual_extended.lo = get(r_memsize_lo);

      Expects (mem.extended.total == mem.actual_extended.total);

      return mem;
    }

    /** CMOS RTC uses 24-hour time format **/
    static bool mode_24_hour() { return reg_b_value & b_24_hr_clock; }

    /** CMOS RTC uses binary representation - otherwise BCD **/
    static bool mode_binary() { return reg_b_value & b_binary_mode; }

    /**
     * A representation of x86 RTC time.
     * With calculation of seconds since epoch and internet timestamp.
     *
     * @note : This class converts CMOS fields to 24-hour format / binary mode
     */
    class Time {
    public:

      /**
       * Data fields of cmos::Time.
       * (For initialization. Recommended practice by CG I.24)
       **/
      struct Fields {
        int     year = 0;
        uint8_t month = 0;
        uint8_t day_of_month = 0;
        uint8_t day_of_week = 0;
        uint8_t hour = 0;
        uint8_t minute = 0;
        uint8_t second = 0;
      };

      uint16_t year() { return f.year; }
      uint8_t month() { return f.month; }
      uint8_t day_of_month() { return f.day_of_month; }
      uint8_t day_of_week() { return f.day_of_week; }
      uint8_t hour() { return f.hour; }
      uint8_t minute() { return f.minute; }
      uint8_t second() { return f.second; }

      /**
       * Populate with data from CMOS.
       *
       * @warning This is a very expensive operation causing several VM-exits
       **/
      Time& hw_update();

      Time() = default;
      Time(Time&) = default;
      Time(Time&&) = default;
      ~Time() = default;
      Time& operator=(Time&) = default;
      Time& operator=(Time&&) = default;

      /** Constructor with all fields. **/
      Time (Time::Fields fields) { f = fields; }

      /** Timestamp string in RFC 3339 format. Aka. Internet Timestamp. */
      std::string to_string();

      static inline bool is_leap_year(uint32_t year) {
        return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
      }

      /** Day-number of current year. **/
      int day_of_year();

      /**
       * POSIX seconds since Epoch
       * http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15
       **/
      uint32_t to_epoch() {
        uint32_t tm_year = year() - 1900;
        uint32_t tm_yday = day_of_year() - 1; // Days since jan. 1st.
        return f.second + f.minute * 60 + f.hour * 3600 + tm_yday * 86400 +
          (tm_year - 70) * 31536000 + ((tm_year - 69) / 4) * 86400 -
          ((tm_year - 1) / 100) * 86400 + ((tm_year + 299) / 400) * 86400;
      }

      Fields fields(){
        return f;
      }

    private:
      Fields f;
    }; // class cmos::Time

    /**
     * Get  current time.
     *
     * @warning: Expensive. This is a very expensive operation causing
     * several VM-exits.
     **/

    static Time now();

  private:
    // The contents of register B (can't always be changed)
    static uint8_t reg_b_value;
    static uint32_t* now_called;
  }; // class cmos

} // hw

#endif
