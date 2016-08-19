// This file is a part of the IncludeOS unikernel - www.includeos.org
//
// Copyright 2015 Oslo and Akershus University College of Applied Sciences
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

#include <os>
#include <iostream>

#include <hw/pit.hpp>
#include <hw/cmos.hpp>


using namespace std::chrono;

void Service::start(const std::string&)
{

  INFO("Test CMOS","Testing C runtime \n");

  CHECKSERT(1 == 1, "CMOS exists");

  unsigned short total;
  unsigned char lowmem, highmem;

  lowmem = cmos::get(0x30);
  highmem = cmos::get(0x31);

  total = lowmem | highmem << 8;

  printf("Total memory: %i \n", total);

  auto regB = cmos::get(cmos::r_status_b);
  printf("RegB: 0x%x Binary mode/daylight: %i, 12-hr-mode: %i \n",
         regB,
         regB & cmos::b_daylight_savings_enabled,
         regB & cmos::b_24_hr_clock);

  // This is done in OS::start() with cmos::init()
  //cmos::set(cmos::r_status_b, cmos::b_24_hr_clock | cmos::b_binary_mode);

  regB = cmos::get(cmos::r_status_b);
  printf("RegB: 0x%x Binary mode/daylight: %i, 12-hr-mode: %i \n",
         regB,
         regB & cmos::b_daylight_savings_enabled,
         regB & cmos::b_24_hr_clock);

  uint32_t wraps = 0;
  uint32_t i = 0;

  while (!cmos::update_in_progress()) {
    i++;
    if (i == 0) {
      wraps++;
      printf("Wrapped %i times, still no update \n", wraps);
      std::cout << cmos::now().to_string() << "\n";
      if (wraps > 10)
        panic("CMOS time didn't update after 10 * 2^32 increments.");
    }
  };

  CHECKSERT(1, "CMOS updated");

  auto tsc_base1 = OS::cycles_since_boot();

  hw::PIT::instance().on_repeated_timeout(1s, [tsc_base1](){
      static auto tsc_base = tsc_base1;
      uint64_t ticks_pr_sec = OS::cycles_since_boot() - tsc_base;
      auto tsc1 = OS::cycles_since_boot();
      auto rtc1 = cmos::now();
      auto tsc2 = OS::cycles_since_boot();
      auto tsc3 = OS::cycles_since_boot();

      printf("<CMOS> Cycles last sec: %llu \n", ticks_pr_sec);
      printf("<CMOS> Reading CMOS Wall-clock took: %llu cycles \n", tsc2 - tsc1);
      printf("<CMOS> RDTSC took: %llu cycles \n", tsc3 - tsc2);

      printf("\n");
      printf("Internet timestamp: %s\n",rtc1.to_string().c_str());
      printf("Seconds since Epoch: %i\n",rtc1.to_epoch());
      printf("Day of year: %i\n", rtc1.day_of_year());
      printf("-------------------------------- \n\n");
      tsc_base = OS::cycles_since_boot();

    });
}
