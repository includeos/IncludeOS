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
//#include <chrono>
#include <hw/ioport.hpp>
#include <hw/pit.hpp>

static const uint8_t cmos_out = 0x70;
static const uint8_t cmos_in = 0x71;

static const uint8_t cmos_no_nmi = 0x80;

static const uint8_t cmos_sec = 0x0;
static const uint8_t cmos_min = 0x2;
static const uint8_t cmos_hrs = 0x4;
static const uint8_t cmos_day = 0x7;
static const uint8_t cmos_month = 0x8;
static const uint8_t cmos_year = 0x9;
static const uint8_t cmos_cent = 0x48;


uint8_t cmos_get(uint8_t reg) {
  hw::outb(cmos_out, reg | cmos_no_nmi);
  return hw::inb(cmos_in);
}


using namespace std::chrono;

void Service::start()
{

  INFO("Test CMOS","Testing C runtime \n");

  CHECKSERT(1 == 1, "CMOS exists");

  unsigned short total;
  unsigned char lowmem, highmem;

  lowmem = cmos_get(0x30);
  highmem = cmos_get(0x31);

  total = lowmem | highmem << 8;

  printf("Total memory: %i \n", total);


  hw::PIT::instance().onRepeatedTimeout(1s, [](){
      auto cent = cmos_get(cmos_cent);
      auto year = cmos_get(cmos_year);
      auto month = cmos_get(cmos_month);
      auto day = cmos_get(cmos_day);
      auto hrs = cmos_get(cmos_hrs);
      auto min = cmos_get(cmos_min);
      auto sec = cmos_get(cmos_sec);

      printf("(cent: %i) %x-%x-%x %x:%x:%x \n",
             cent, day, month, year, hrs, min, sec);

    });


}
