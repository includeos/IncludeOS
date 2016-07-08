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

//#define DEBUG // Debug supression

#include <os>
#include <kernel/irq_manager.hpp>
#include <hw/serial.hpp>
#include <list>
#include <net/inet4>
#include <vector>

using namespace std;
using namespace net;

std::unique_ptr<net::Inet4<VirtioNet> > inet;

/**

   Test the PIC IRQ subsystem
   We're going to use the network- and keyboard interrupts for now, using UDP
   to trigger the NIC irq.

**/
void Service::start()
{

  // Serial
  auto& com1 = hw::Serial::port<1>();

  // Timers
  auto& time = hw::PIT::instance();

  // Network
  auto& eth0 = hw::Dev::eth<0,VirtioNet>();
  auto& mac = eth0.mac();
  auto& inet = *new net::Inet4<VirtioNet>(eth0, // Device
    { mac.part[2],mac.part[3],mac.part[4],mac.part[5] }, // IP
    { 255,255,0,0 }, // Netmask
    { 10,0,0,1 } );  // Gateway

  printf("Service IP address: %s \n", inet.ip_addr().str().c_str());

  // UDP
  UDP::port_t port = 4242;
  auto& sock = inet.udp().bind(port);

  sock.on_read(
  [&sock] (UDP::addr_t addr, UDP::port_t port,
          const char* data, int len)
  {
    auto str = std::string(data,len);
    str[len -1] = 0;
    CHECK(1,"UDP received '%s'", str.c_str());
    // send the same thing right back!
    sock.sendto(addr, port, data, len);
  });

  auto& irq_man = IRQ_manager::cpu(0);
  irq_man.subscribe(1, [](){
      CHECK(1,"IRQ 1 delegate called once");
    });

  irq_man.subscribe(3, [](){
      static int irq3_count {1};
      CHECK(1,"IRQ 3 delegate called %i times ", irq3_count++);
    });

  INFO("IRQ test","Soft-triggering IRQ's");
  // This will trigger IRQ-handlers,
  // but default handlers should only yield output in debug-mode
  // unless they have subscribers, in which case the subscribers should get called later

  asm("int $32"); // Expect IRQ 0 handler
  asm("int $33"); // Expect IRQ 1 handler
  //asm("int $34"); // IRQ 2 is slave so no handler
  asm("int $35"); // Expect IRQ 3 handler
  asm("int $35"); // Expect IRQ 3 handler
  asm("int $35"); // Expect IRQ 3 handler
  asm("int $36"); // Expect IRQ 4 handler
  asm("int $37"); // ... etc.
  asm("int $38");
  asm("int $39");
  asm("int $40");
  asm("int $41");
  asm("int $42");
  asm("int $43");
  asm("int $44");
  asm("int $45");
  asm("int $46");
  asm("int $47"); // Expect IRQ 15 handler
  asm("int $48"); // Expect "unexpected IRQ"
  asm("int $49"); // Expect "unexpected IRQ"


  com1.enable_interrupt();


  /**
     A custom IRQ-handler for the serial port
     It doesn't send eoi, but it should work anyway
     since we're using auto-EOI-mode for IRQ < 8 (master)
  */
  irq_man.subscribe(4, [](){
      uint16_t serial_port1 = 0x3F8;
      //IRQ_manager::eoi(4);
      char byte = 0;
      while (hw::inb(serial_port1 + 5) & 1)
        byte = hw::inb(serial_port1);

      CHECK(1,"Serial port (IRQ 4) received '%c'", byte);
    });
  INFO("Test Serial Port", "Calling Serial Port Test");
  printf("trigger_test_serial_port\n");


  /*
    IRQ_manager::subscribe(11,[](){
    // Calling eoi here will turn the IRQ line on and loop forever.
    IRQ_manager::eoi(11);
    INFO("IRQ","Network IRQ\n");
    });*/


  // Enabling a timer causes freeze in debug mode, for some reason
  time.on_repeated_timeout(1s, [](){
      static int time_counter = 0;
      CHECK(1,"Time %i", ++time_counter);

    });


  INFO("IRQ test","Expect IRQ subscribers to get called now ");
}
