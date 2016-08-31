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
#include <net/inet4>

using namespace std::chrono;
#include <kernel/elf.hpp>
#include <profile>

extern void print_heap_info();
extern void print_backtrace();

void print_tcp_status() {
  auto& inet = net::Inet4::stack<0>();
  printf("<Service> TCP STATUS: %u\n", inet.tcp().active_connections());
  print_heap_info();
  print_backtrace();
}

uint8_t bullshit[65536*4];
void __validate_bullshit(char const* where)
{
  extern void __panic_failure(char const* where, size_t);
  for (size_t i = 0; i < sizeof(bullshit); i++)
      if (bullshit[i]) {
        printf("\tbullshit[%u] = %#x\n", i, bullshit[i]);
        __panic_failure(where, i);
      }
}

static size_t counter = 0;
void do_nothing_useful()
{
  __validate_bullshit("validate_bullshit: begin do_nothing_useful()");
  static bool mode_free = false;
  const size_t NUM_POINTERS = 24000;
  static uint8_t* ptrs[NUM_POINTERS];
  
  printf("Validing heap...");
  //__validate_backtrace("BEFORE do_nothing_useful()");
  counter++;
  
  if (!mode_free) {
    printf("Allocing...\n");
    // allocate all pointers
    for (size_t i = 0; i < NUM_POINTERS; i++)
    {
      //__validate_backtrace("BEFORE NEW", i);
      const size_t BUFFER_SIZE = 4096;
      ptrs[i] = new uint8_t[BUFFER_SIZE];
      //__validate_backtrace("BEFORE MEMSET", i);
      memset(ptrs[i], 0, BUFFER_SIZE);
      
      // create empty packet
      //__validate_backtrace("BEFORE Packet()", i);
      new (ptrs[i]) net::Packet(BUFFER_SIZE, 0);
    }
  } else {
    // delete all pointers
    printf("Deleting...\n");
    for (size_t i = 0; i < NUM_POINTERS; i++) {
      //__validate_backtrace("BEFORE DELETE", i);
      delete[] ptrs[i];
      ptrs[i] = nullptr;
    }
  }
  // toggle mode
  mode_free = !mode_free;
  __validate_bullshit("validate_bullshit: endof do_nothing_useful()");
  printf("done\n");
}

extern "C" {
  extern char _end;
  extern void _start();
}

#include <hw/cpu.hpp>
#include <timers>

void Service::start(const std::string&)
{
  //printf("static array @ %p size is %u\n", bullshit, sizeof(bullshit));
  //memset(bullshit, 0, sizeof(bullshit));
  //__validate_bullshit("validate_bullshit begin Service::start()");
  // heap validation test
  //hw::PIT::instance().on_repeated_timeout(200ms, do_nothing_useful);
  //__validate_bullshit("validate_bullshit endof Service::start()");
  
  //begin_stack_sampling(200);
  // print sampling results every 5 seconds
  //hw::PIT::instance().on_repeated_timeout(500ms, print_stack_sampling);

  //Timers::create(500ms, 500ms,
  //[] {
  //  print_heap_info();
  //  printf("bufstore packets: %u\n", inet->buffers_available());
  //});

  printf("Creating repeating timer for 500ms\n");
  static int C = 0;
  for (int i = 0; i < 5000; i++)
  Timers::periodic(500ms, 500ms,
  [] (auto) {
    printf("beep\t");
    C++;
    if (C == 10) printf("\n");
  });
  
  // boilerplate
  auto& inet = net::Inet4::stack<0>();

  // Set up a TCP server on port 80
  auto& server = inet.tcp().bind(80);
  server.on_connect(
  [] (auto conn) {
    conn->close();
  });
  
  auto& echo_server = inet.udp().bind(66);
  echo_server.on_read(
  [&echo_server] (auto addr, auto port,
                  const char* data, size_t len)
  {
    // send the same thing right back!
    echo_server.sendto(addr, port, data, len,
    [len] {
      //INFO("UDP", "Echo reply len=%u", len);
    });
  });
}
